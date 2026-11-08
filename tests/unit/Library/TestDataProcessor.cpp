#include "DataProcessor.hpp"
#include "EyeTracking/EyeTrackingCalibrationModel.hpp"
#include "EyeTracking/EyeTrackingDetector.hpp"
#include "HeartRate/UFeelHeartRateStatus.hpp"
#include "SpeechToText/TestDoubles.hpp"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"

#include <chrono>
#include <cstdint>
#include <functional>
#include <gtest/gtest.h>
#include <stdexcept>
#include <thread>
#include <utility>

#ifndef DATA_PROCESSOR_MODEL_DIR_PATH
#define DATA_PROCESSOR_MODEL_DIR_PATH ""
#endif // DATA_PROCESSOR_MODEL_DIR_PATH

namespace
{

auto createProcessor(
    int cameraIndex = 0
) -> DataProcessor
{
    return {DATA_PROCESSOR_MODEL_DIR_PATH, "fake-model", cameraIndex};
}

auto createProcessor(
    std::function<bool(cv::Mat&)> frameReader
) -> DataProcessor
{
    return {DATA_PROCESSOR_MODEL_DIR_PATH, "fake-model", std::move(frameReader)};
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
    auto processor = createProcessor(999999);
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
    EXPECT_EQ(processor.getHeartRate().status, HeartRateStatus::NoData);
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
    EXPECT_EQ(processor.getHeartRate().status, HeartRateStatus::NoData);
}

TEST(
    DataProcessorTest, SpeechToggleUpdatesStoredResult
)
{
    speech_to_text_test::VoskFake::reset();
    auto processor = createProcessor();

    processor.toggleSpeechToText(true);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!speech_to_text_test::VoskFake::hasProcessedAudio() &&
           std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    ASSERT_TRUE(speech_to_text_test::VoskFake::hasProcessedAudio());
    EXPECT_EQ(processor.updateSpeechToText(), DataProcessorStatus::Ok);
    EXPECT_EQ(processor.getSpeechToText().status, SpeechToTextStatus::Valid);
    EXPECT_EQ(processor.getSpeechToText().text, "hello");
    EXPECT_EQ(processor.updateFromFrame(Speech, cv::Mat{}), DataProcessorStatus::Ok);
    EXPECT_EQ(processor.getSpeechToText().status, SpeechToTextStatus::Valid);

    processor.toggleSpeechToText(false);
    EXPECT_EQ(processor.getSpeechToText().status, SpeechToTextStatus::Disabled);
    EXPECT_TRUE(processor.getSpeechToText().text.empty());
}

TEST(
    DataProcessorTest, ReportsNoSpeechWhenRecognitionHasNoText
)
{
    speech_to_text_test::VoskFake::reset();
    speech_to_text_test::VoskFake::setPartialResult(R"({"partial": ""})");
    speech_to_text_test::VoskFake::setFinalizedResult(R"({"text": ""})");
    auto processor = createProcessor();

    processor.toggleSpeechToText(true);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!speech_to_text_test::VoskFake::hasProcessedAudio() &&
           std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    ASSERT_TRUE(speech_to_text_test::VoskFake::hasProcessedAudio());
    EXPECT_EQ(processor.updateSpeechToText(), DataProcessorStatus::Ok);
    EXPECT_EQ(processor.getSpeechToText().status, SpeechToTextStatus::NoSpeech);
    EXPECT_EQ(processor.updateFromFrame(Speech, cv::Mat{}), DataProcessorStatus::Ok);
    EXPECT_EQ(processor.getSpeechToText().status, SpeechToTextStatus::NoSpeech);
    processor.toggleSpeechToText(false);
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
    const cv::Mat frame(480, 640, CV_8UC3, cv::Scalar(0, 0, 0));

    EXPECT_EQ(processor.updateFromFrame(Emotions | EyeTracking, frame), DataProcessorStatus::Ok);
    EXPECT_EQ(processor.getFrame().size(), frame.size());
    EXPECT_EQ(processor.getEmotions().status, EmotionStatus::Valid);
    EXPECT_EQ(processor.getEyeTracking().status, EyeTrackingStatus::FaceUnavailable);

    EXPECT_EQ(processor.updateEmotionsFromFrame(frame), DataProcessorStatus::Ok);
    EXPECT_EQ(processor.getEmotionsFromFrame(frame).status, EmotionStatus::Valid);
    EXPECT_EQ(processor.updateEyeTrackingFromFrame(frame), DataProcessorStatus::Ok);
    EXPECT_EQ(processor.getEyeTrackingFromFrame(frame).status, EyeTrackingStatus::FaceUnavailable);
}

TEST(
    DataProcessorTest, UpdatesFeaturesFromInjectedFrameReader
)
{
    const cv::Mat frame(480, 640, CV_8UC3, cv::Scalar(0, 0, 0));
    auto processor = createProcessor([&frame](cv::Mat& capturedFrame) -> bool {
        capturedFrame = frame.clone();
        return true;
    });

    EXPECT_EQ(processor.updateFrame(), DataProcessorStatus::Ok);
    EXPECT_EQ(processor.getFrame().size(), frame.size());
    EXPECT_EQ(processor.updateEmotions(), DataProcessorStatus::Ok);
    EXPECT_EQ(processor.getEmotions().status, EmotionStatus::Valid);
    EXPECT_EQ(processor.updateEyeTracking(), DataProcessorStatus::Ok);
    EXPECT_EQ(processor.getEyeTracking().status, EyeTrackingStatus::FaceUnavailable);
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
