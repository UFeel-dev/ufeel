#include "DataProcessor.hpp"
#include "EyeTracking/EyeTrackingCalibrationModel.hpp"
#include "EyeTracking/EyeTrackingDetector.hpp"
#include "HeartRate/HeartRateSource.hpp"
#include "HeartRate/UFeelHeartRateStatus.hpp"
#include "Library/HeartRate/TestSupport/HeartRateSourceFake.hpp"
#include "Library/SpeechToText/TestSupport/WavAudioSource.hpp"
#include "Library/TestSupport/Fixtures.hpp"
#include "SpeechToText/AudioCapture.hpp"
#include "SpeechToText/AudioSource.hpp"
#include "SpeechToText/TestDoubles.hpp"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"

#include <cstdint>
#include <functional>
#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#ifndef DATA_PROCESSOR_MODEL_DIR_PATH
#define DATA_PROCESSOR_MODEL_DIR_PATH ""
#endif // DATA_PROCESSOR_MODEL_DIR_PATH

namespace
{

constexpr HeartRateResult kHeartRate{.bpm = 72, .status = HeartRateStatus::Valid};

auto makeAudioSource() -> std::unique_ptr<WavAudioSource>
{
    return std::make_unique<WavAudioSource>(std::vector<int16_t>(AudioCapture::framesPerBuffer));
}

auto makeDevices(
    std::function<bool(cv::Mat&)> frameReader = frameSequence({}),
    std::unique_ptr<AudioSource> audioSource = makeAudioSource()
) -> DataProcessorDevices
{
    DataProcessorDevices devices{
        .frameReader = std::move(frameReader),
        .audioSource = std::move(audioSource),
    };
    devices.heartRateSources.push_back(std::make_unique<HeartRateSourceFake>(kHeartRate));
    return devices;
}

auto createProcessor(
    DataProcessorDevices devices = makeDevices()
) -> DataProcessor
{
    return {DATA_PROCESSOR_MODEL_DIR_PATH, "fake-model", std::move(devices)};
}

auto startSpeech() -> std::unique_ptr<DataProcessor>
{
    auto audioSource = makeAudioSource();
    auto* audio = audioSource.get();
    auto processor = std::make_unique<DataProcessor>(
        DATA_PROCESSOR_MODEL_DIR_PATH,
        "fake-model",
        makeDevices(frameSequence({}), std::move(audioSource))
    );
    processor->toggleSpeechToText(true);
    EXPECT_TRUE(audio->waitUntilDrained());
    return processor;
}

} // namespace

TEST(
    DataProcessorTest, RejectsInvalidDetectionFlags
)
{
    auto processor = createProcessor();

    EXPECT_EQ(
        processor.updateFromFrame(1U << 8U, cv::Mat{}), DataProcessorStatus::InvalidDetectionType
    );
    EXPECT_EQ(processor.update(1U << 8U), DataProcessorStatus::InvalidDetectionType);
#if !UFEEL_ENABLE_EMOTIONS
    EXPECT_EQ(processor.update(Emotions), DataProcessorStatus::UnsupportedFeature);
#endif
#if !UFEEL_ENABLE_EYE_TRACKING
    EXPECT_EQ(processor.update(EyeTracking), DataProcessorStatus::UnsupportedFeature);
#endif
#if !UFEEL_ENABLE_SPEECH_TO_TEXT
    EXPECT_EQ(processor.update(Speech), DataProcessorStatus::UnsupportedFeature);
#endif
#if !UFEEL_ENABLE_HEART_RATE
    EXPECT_EQ(processor.update(HeartRate), DataProcessorStatus::UnsupportedFeature);
#endif
}

TEST(
    DataProcessorTest, RequiresInjectedFrameReader
)
{
    EXPECT_THROW(
        DataProcessor(DATA_PROCESSOR_MODEL_DIR_PATH, "fake-model", std::function<bool(cv::Mat&)>{}),
        std::invalid_argument
    );
}

TEST(
    DataProcessorTest, ReportsUnavailableCameraFrames
)
{
    auto devices = makeDevices();
    devices.frameReader = nullptr;
    devices.cameraIndex = 999999;
    auto processor = createProcessor(std::move(devices));
    const cv::Mat frame(8, 8, CV_8UC3, cv::Scalar(0, 0, 0));
    ASSERT_EQ(processor.setFrame(frame), DataProcessorStatus::Ok);

    EXPECT_EQ(processor.updateFrame(), DataProcessorStatus::FrameUnavailable);
    EXPECT_TRUE(processor.getFrame().empty());
    EXPECT_EQ(processor.update(Emotions | EyeTracking), DataProcessorStatus::FrameUnavailable);
    EXPECT_EQ(processor.getEmotions().status, EmotionStatus::FrameUnavailable);
    EXPECT_EQ(processor.getEyeTracking().status, EyeTrackingStatus::FrameUnavailable);
    EXPECT_EQ(processor.updateEmotions(), DataProcessorStatus::FrameUnavailable);
    EXPECT_EQ(processor.updateEyeTracking(), DataProcessorStatus::FrameUnavailable);
}

TEST(
    DataProcessorTest, OwnsFramesAndRejectsEmptyReplacement
)
{
    auto processor = createProcessor();
    cv::Mat input(2, 2, CV_8UC1, cv::Scalar(17));

    ASSERT_EQ(processor.setFrame(input), DataProcessorStatus::Ok);
    input.setTo(cv::Scalar(99));

    const auto stored = processor.getFrame();
    ASSERT_FALSE(stored.empty());
    EXPECT_EQ(stored.at<std::uint8_t>(0, 0), 17U);
    EXPECT_EQ(processor.setFrame(cv::Mat{}), DataProcessorStatus::InvalidFrame);
    EXPECT_EQ(processor.getFrame().at<std::uint8_t>(0, 0), 17U);
}

TEST(
    DataProcessorTest, FrameFreeOperationsDoNotRequireAnImage
)
{
    auto processor = createProcessor();

    EXPECT_EQ(processor.updateFromFrame(None, cv::Mat{}), DataProcessorStatus::Ok);
    EXPECT_EQ(processor.updateFromFrame(Speech, cv::Mat{}), DataProcessorStatus::Ok);
    EXPECT_EQ(processor.updateSpeechToText(), DataProcessorStatus::Ok);
    EXPECT_EQ(processor.getSpeechToText().status, SpeechToTextStatus::Disabled);
    EXPECT_TRUE(processor.getSpeechToText().text.empty());
    EXPECT_EQ(processor.updateHeartRate(), DataProcessorStatus::Ok);
    EXPECT_EQ(processor.getHeartRate().bpm, kHeartRate.bpm);
}

TEST(
    DataProcessorTest, RoutesEyeTrackingAndHeartRateDetectionFlags
)
{
    auto processor = createProcessor();
    const cv::Mat frame(480, 640, CV_8UC3, cv::Scalar(0, 0, 0));

    EXPECT_EQ(processor.updateFromFrame(EyeTracking, frame), DataProcessorStatus::Ok);
    EXPECT_EQ(processor.getEyeTracking().status, EyeTrackingStatus::FaceUnavailable);

    EXPECT_EQ(processor.updateFromFrame(HeartRate, cv::Mat{}), DataProcessorStatus::Ok);
    EXPECT_EQ(processor.getHeartRate().status, kHeartRate.status);
    EXPECT_EQ(processor.getHeartRate().bpm, kHeartRate.bpm);
}

TEST(
    DataProcessorTest, SpeechToggleUpdatesStoredResult
)
{
    speech_to_text_test::VoskFake::reset();
    auto processor = startSpeech();

    EXPECT_EQ(processor->updateSpeechToText(), DataProcessorStatus::Ok);
    EXPECT_EQ(processor->getSpeechToText().status, SpeechToTextStatus::Valid);
    EXPECT_EQ(processor->getSpeechToText().text, "hello");
    EXPECT_EQ(processor->updateFromFrame(Speech, cv::Mat{}), DataProcessorStatus::Ok);
    EXPECT_EQ(processor->getSpeechToText().status, SpeechToTextStatus::Valid);

    processor->toggleSpeechToText(false);
    EXPECT_EQ(processor->getSpeechToText().status, SpeechToTextStatus::Disabled);
    EXPECT_TRUE(processor->getSpeechToText().text.empty());
}

TEST(
    DataProcessorTest, ReportsNoSpeechWhenRecognitionHasNoText
)
{
    speech_to_text_test::VoskFake::reset();
    speech_to_text_test::VoskFake::setPartialResult(R"({"partial": ""})");
    speech_to_text_test::VoskFake::setFinalizedResult(R"({"text": ""})");
    auto processor = startSpeech();

    EXPECT_EQ(processor->updateSpeechToText(), DataProcessorStatus::Ok);
    EXPECT_EQ(processor->getSpeechToText().status, SpeechToTextStatus::NoSpeech);
    EXPECT_EQ(processor->updateFromFrame(Speech, cv::Mat{}), DataProcessorStatus::Ok);
    EXPECT_EQ(processor->getSpeechToText().status, SpeechToTextStatus::NoSpeech);
    processor->toggleSpeechToText(false);
}

TEST(
    DataProcessorTest, FrameFailureUpdatesOnlyRequestedFeatureResults
)
{
    auto processor = createProcessor();
    const cv::Mat frame(480, 640, CV_8UC3, cv::Scalar(0, 0, 0));
    ASSERT_EQ(processor.updateFromFrame(Emotions | EyeTracking, frame), DataProcessorStatus::Ok);

    EXPECT_EQ(
        processor.updateFromFrame(Emotions | EyeTracking, cv::Mat{}),
        DataProcessorStatus::InvalidFrame
    );
    EXPECT_EQ(processor.getEmotions().status, EmotionStatus::FrameUnavailable);
    EXPECT_EQ(processor.getEyeTracking().status, EyeTrackingStatus::FrameUnavailable);

    ASSERT_EQ(processor.updateEmotionsFromFrame(frame), DataProcessorStatus::Ok);
    ASSERT_EQ(processor.updateEyeTrackingFromFrame(frame), DataProcessorStatus::Ok);
    EXPECT_EQ(processor.updateFromFrame(Emotions, cv::Mat{}), DataProcessorStatus::InvalidFrame);
    EXPECT_EQ(processor.getEmotions().status, EmotionStatus::FrameUnavailable);
    EXPECT_EQ(processor.getEyeTracking().status, EyeTrackingStatus::FaceUnavailable);
}

TEST(
    DataProcessorTest, EmptyFramesSetFeatureResultsToUnavailable
)
{
    auto processor = createProcessor();

    EXPECT_EQ(processor.updateEmotionsFromFrame(cv::Mat{}), DataProcessorStatus::InvalidFrame);
    EXPECT_EQ(processor.getEmotions().status, EmotionStatus::FrameUnavailable);
    EXPECT_EQ(processor.getEmotionsFromFrame(cv::Mat{}).status, EmotionStatus::FrameUnavailable);

    EXPECT_EQ(processor.updateEyeTrackingFromFrame(cv::Mat{}), DataProcessorStatus::InvalidFrame);
    EXPECT_EQ(processor.getEyeTracking().status, EyeTrackingStatus::FrameUnavailable);
    EXPECT_EQ(
        processor.getEyeTrackingFromFrame(cv::Mat{}).status, EyeTrackingStatus::FrameUnavailable
    );
}

TEST(
    DataProcessorTest, ProcessesImageFeaturesFromProvidedFrame
)
{
    auto processor = createProcessor();
    const auto face = loadFaceFrame();
    const auto blank = makeBlankFrame();
    ASSERT_FALSE(face.empty());

    EXPECT_EQ(processor.updateFromFrame(Emotions | EyeTracking, face), DataProcessorStatus::Ok);
    EXPECT_EQ(processor.getFrame().size(), face.size());
    EXPECT_EQ(processor.getEmotions().status, EmotionStatus::Valid);
    EXPECT_EQ(processor.getEyeTracking().status, EyeTrackingStatus::NotCalibrated);

    EXPECT_EQ(processor.updateEmotionsFromFrame(blank), DataProcessorStatus::Ok);
    EXPECT_EQ(processor.getEmotions().status, EmotionStatus::FaceUnavailable);
    EXPECT_EQ(processor.getEmotionsFromFrame(face).status, EmotionStatus::Valid);
    EXPECT_EQ(processor.getEmotionsFromFrame(blank).status, EmotionStatus::FaceUnavailable);
    EXPECT_EQ(processor.updateEyeTrackingFromFrame(blank), DataProcessorStatus::Ok);
    EXPECT_EQ(processor.getEyeTracking().status, EyeTrackingStatus::FaceUnavailable);
    EXPECT_EQ(processor.getEyeTrackingFromFrame(blank).status, EyeTrackingStatus::FaceUnavailable);
}

TEST(
    DataProcessorTest, UpdatesFeaturesFromInjectedFrameReader
)
{
    const auto face = loadFaceFrame();
    ASSERT_FALSE(face.empty());
    auto processor = createProcessor(makeDevices(frameSequence({face, makeBlankFrame()})));

    EXPECT_EQ(processor.updateFrame(), DataProcessorStatus::Ok);
    EXPECT_EQ(processor.getFrame().size(), face.size());
    EXPECT_EQ(processor.updateEmotions(), DataProcessorStatus::Ok);
    EXPECT_EQ(processor.getEmotions().status, EmotionStatus::FaceUnavailable);
    EXPECT_EQ(processor.updateEyeTracking(), DataProcessorStatus::FrameUnavailable);
    EXPECT_EQ(processor.getEyeTracking().status, EyeTrackingStatus::FrameUnavailable);
}

TEST(
    DataProcessorTest, ForwardsEyeTrackingCalibrationOperations
)
{
    auto processor = createProcessor();

    EXPECT_FALSE(processor.setEyeTrackingCalibrationTarget(0.5F, 0.5F));
    processor.beginEyeTrackingCalibration();
    EXPECT_FALSE(processor.setEyeTrackingCalibrationTarget(-0.1F, 0.5F));
    EXPECT_FALSE(processor.setEyeTrackingCalibrationTarget(0.5F, 1.1F));
    EXPECT_TRUE(processor.setEyeTrackingCalibrationTarget(0.5F, 0.5F));

    const auto progress = processor.getEyeTrackingCalibrationProgress();
    EXPECT_EQ(progress.targetCount, 1U);
    EXPECT_EQ(progress.targetsWithEnoughSamples, 0U);
    EXPECT_FALSE(processor.finishEyeTrackingCalibration());
    EXPECT_NE(
        processor.getEyeTrackingCalibrationDiagnostics().failureReason,
        CalibrationFailureReason::None
    );

    processor.beginEyeTrackingCalibration();
    processor.cancelEyeTrackingCalibration();
    EXPECT_FALSE(processor.finishEyeTrackingCalibration());
    EXPECT_FALSE(processor.saveEyeTrackingCalibrationProfile("/missing/path/profile.json"));
    EXPECT_FALSE(processor.loadEyeTrackingCalibrationProfile("/missing/path/profile.json"));
}
