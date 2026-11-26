#include "Adapters/cpp/TestSupport/FakeProcessor.hpp"
#include "DataProcessor.hpp"
#include "EyeTracking/EyeTrackingDetector.hpp"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"
#include "ufeel/Model/Calibration.hpp"
#include "ufeel/Model/Detection.hpp"
#include "ufeel/Model/EmotionResult.hpp"
#include "ufeel/Model/EyeTrackingResult.hpp"
#include "ufeel/Model/Flags.hpp"
#include "ufeel/Model/Frame.hpp"
#include "ufeel/Model/HeartRateResult.hpp"
#include "ufeel/Model/SpeechToTextResult.hpp"
#include "ufeel/Model/Status.hpp"
#include "ufeel/Native/Error.hpp"
#include "ufeel/Native/Processor.hpp"

#include <cstdint>
#include <filesystem>
#include <gtest/gtest.h>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace
{

template <typename Function>
auto errorStatus(
    Function&& function
) -> std::optional<ufeel::Status>
{
    try
    {
        std::forward<Function>(function)();
    }
    catch (const ufeel::Error& error)
    {
        return error.status();
    }
    return std::nullopt;
}

} // namespace

TEST(
    ProcessorTest, RejectsInvalidConstructionArguments
)
{
    EXPECT_THROW(static_cast<void>(ufeel::Processor::adopt(nullptr)), std::invalid_argument);
    EXPECT_THROW(ufeel::Processor("", "speech"), std::invalid_argument);
    EXPECT_THROW(ufeel::Processor("models", std::filesystem::path()), std::invalid_argument);
}

TEST(
    ProcessorTest, ThrowsWhenUsedAfterBeingMoved
)
{
    auto [backend, processor] = makeFakeProcessor();
    const ufeel::Processor owner = std::move(processor);

    // NOLINTNEXTLINE(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
    EXPECT_THROW(static_cast<void>(processor.heartRate()), std::logic_error);
}

TEST(
    ProcessorTest, ReturnsUpdateStatuses
)
{
    auto [backend, processor] = makeFakeProcessor();

    EXPECT_EQ(processor.update(ufeel::DetectionType::HeartRate), ufeel::Status::Ok);

    backend->updateStatus = DataProcessorStatus::UnsupportedFeature;
    EXPECT_EQ(processor.update(ufeel::DetectionType::HeartRate), ufeel::Status::UnsupportedFeature);

    backend->updateFrameStatus = DataProcessorStatus::FrameUnavailable;
    EXPECT_EQ(processor.updateFrame(), ufeel::Status::InvalidFrame);
}

TEST(
    ProcessorTest, UpdatesFromCallerFrames
)
{
    auto [backend, processor] = makeFakeProcessor();
    const ufeel::Frame frame{
        .data = std::vector<std::uint8_t>(12, 0),
        .width = 2,
        .height = 2,
        .channels = 3,
        .stride = 6,
    };
    const ufeel::Frame truncated{
        .data = {1, 2},
        .width = 2,
        .height = 2,
        .channels = 3,
        .stride = 6,
    };

    EXPECT_EQ(processor.updateFromFrame(ufeel::DetectionType::Emotions, frame), ufeel::Status::Ok);
    EXPECT_EQ(
        processor.updateFromFrame(ufeel::DetectionType::Emotions, truncated),
        ufeel::Status::InvalidFrame
    );
}

TEST(
    ProcessorTest, CopiesTheLatestFrame
)
{
    auto [backend, processor] = makeFakeProcessor();
    backend->frame = cv::Mat(2, 3, CV_8UC3, cv::Scalar(1, 2, 3));

    ufeel::Frame frame;
    ASSERT_TRUE(processor.copyFrame(frame));
    EXPECT_EQ(processor.frameSize(), (ufeel::FrameSize{.width = 3, .height = 2}));
    EXPECT_EQ(frame.width, 3);
    EXPECT_EQ(frame.height, 2);
    EXPECT_EQ(frame.channels, 3);
    EXPECT_EQ(frame.stride, 9);
    EXPECT_EQ(
        frame.data,
        std::vector<std::uint8_t>({1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3, 1, 2, 3})
    );
}

TEST(
    ProcessorTest, CopiesTheLatestFrameAsBgra
)
{
    auto [backend, processor] = makeFakeProcessor();
    backend->frame = cv::Mat(1, 2, CV_8UC3, cv::Scalar(1, 2, 3));
    std::vector<std::uint8_t> pixels(8);

    ASSERT_TRUE(processor.copyFrameBgra(pixels, 8, {.width = 2, .height = 1}));
    EXPECT_EQ(pixels, std::vector<std::uint8_t>({1, 2, 3, 255, 1, 2, 3, 255}));
    EXPECT_FALSE(processor.copyFrameBgra(pixels, 4, {.width = 1, .height = 1}));
    EXPECT_THROW(
        static_cast<void>(processor.copyFrameBgra(pixels, 8, {.width = 2, .height = 2})),
        std::invalid_argument
    );
    EXPECT_THROW(
        static_cast<void>(processor.copyFrameBgra(pixels, 4, {.width = 2, .height = 1})),
        std::invalid_argument
    );
}

TEST(
    ProcessorTest, ReportsMissingFrames
)
{
    auto [backend, processor] = makeFakeProcessor();
    ufeel::Frame frame;
    std::vector<std::uint8_t> pixels(4);

    EXPECT_EQ(processor.frameSize(), std::nullopt);
    EXPECT_FALSE(processor.copyFrame(frame));
    EXPECT_FALSE(processor.copyFrameBgra(pixels, 4, {.width = 1, .height = 1}));
}

TEST(
    ProcessorTest, ReadsEmotions
)
{
    auto [backend, processor] = makeFakeProcessor();
    backend->emotions = ::EmotionResult{
        .status = ::EmotionStatus::Valid,
        .emotions = {{"happy", 0.8F}, {"sad", 0.1F}},
    };

    const ufeel::EmotionResult emotions = processor.emotions();

    EXPECT_EQ(emotions.status, ufeel::EmotionStatus::Valid);
    EXPECT_EQ(emotions.scores.size(), 2U);
    EXPECT_FLOAT_EQ(emotions.scores.at("happy"), 0.8F);
    EXPECT_EQ(emotions.dominant(), ufeel::Emotion::Happy);
}

TEST(
    ProcessorTest, ReadsUnavailableEmotions
)
{
    auto [backend, processor] = makeFakeProcessor();
    backend->emotions = ::EmotionResult{.status = ::EmotionStatus::FaceUnavailable, .emotions = {}};

    const ufeel::EmotionResult emotions = processor.emotions();

    EXPECT_EQ(emotions.status, ufeel::EmotionStatus::FaceUnavailable);
    EXPECT_TRUE(emotions.scores.empty());
}

TEST(
    ProcessorTest, ReadsEyeTracking
)
{
    auto [backend, processor] = makeFakeProcessor();
    backend->eyeTracking.gazeX = -0.5F;
    backend->eyeTracking.eyeState = EyeStateTracked | EyeStateLeftClosed;
    backend->eyeTracking.blinkCount = 4;

    const ufeel::EyeTrackingResult eyeTracking = processor.eyeTracking();

    EXPECT_EQ(eyeTracking.status, ufeel::EyeTrackingStatus::Valid);
    EXPECT_FLOAT_EQ(eyeTracking.estimatedX, 0.2F);
    EXPECT_FLOAT_EQ(eyeTracking.estimatedY, 0.7F);
    EXPECT_FLOAT_EQ(eyeTracking.recordedX, 0.3F);
    EXPECT_FLOAT_EQ(eyeTracking.confidence, 0.9F);
    EXPECT_EQ(
        eyeTracking.directions, ufeel::EyeTrackingDirection::Left | ufeel::EyeTrackingDirection::Up
    );
    EXPECT_FLOAT_EQ(eyeTracking.gazeX, -0.5F);
    EXPECT_TRUE(eyeTracking.isTracked());
    EXPECT_FALSE(eyeTracking.areEyesClosed());
    EXPECT_EQ(eyeTracking.blinkCount, 4U);
}

TEST(
    ProcessorTest, RejectsUnknownNativeStatuses
)
{
    auto [backend, processor] = makeFakeProcessor();
    backend->eyeTracking.status = static_cast<::EyeTrackingStatus>(2);

    EXPECT_EQ(
        errorStatus([&processor] -> void { static_cast<void>(processor.eyeTracking()); }),
        ufeel::Status::InternalError
    );
}

TEST(
    ProcessorTest, ReadsSpeechToText
)
{
    auto [backend, processor] = makeFakeProcessor();

    const ufeel::SpeechToTextResult speech = processor.speechToText();

    EXPECT_TRUE(speech.isValid());
    EXPECT_EQ(speech.text, "recognized");
}

TEST(
    ProcessorTest, ReadsHeartRate
)
{
    auto [backend, processor] = makeFakeProcessor();

    const ufeel::HeartRateResult heartRate = processor.heartRate();

    EXPECT_TRUE(heartRate.isValid());
    EXPECT_EQ(heartRate.bpm, 73U);
}

TEST(
    ProcessorTest, ThrowsWhenTheNativeLibraryFails
)
{
    auto [backend, processor] = makeFakeProcessor();
    backend->throwOnHeartRate = true;

    EXPECT_EQ(
        errorStatus([&processor] -> void { static_cast<void>(processor.heartRate()); }),
        ufeel::Status::InternalError
    );
}

TEST(
    ProcessorTest, ControlsSpeechToTextAndHeadWeight
)
{
    auto [backend, processor] = makeFakeProcessor();

    processor.setSpeechToTextEnabled(true);
    processor.setEyeTrackingHeadWeight(0.5F);

    EXPECT_TRUE(backend->speechEnabled);
    EXPECT_FLOAT_EQ(backend->headWeight, 0.5F);
    EXPECT_EQ(
        errorStatus([&processor] -> void { processor.setEyeTrackingHeadWeight(2.0F); }),
        ufeel::Status::InvalidArgument
    );
}

TEST(
    ProcessorTest, ListsAndSelectsMicrophones
)
{
    auto [backend, processor] = makeFakeProcessor();

    const auto microphones = processor.microphones();

    ASSERT_EQ(microphones.size(), 2U);
    EXPECT_EQ(microphones.at(1).id, "bluez_input.headset");
    EXPECT_EQ(microphones.at(1).name, "Headset");
    EXPECT_TRUE(microphones.at(1).isDefault);
    EXPECT_TRUE(processor.selectMicrophone("alsa_input.dmic"));
    EXPECT_EQ(backend->selectedMicrophone, "alsa_input.dmic");
    EXPECT_FALSE(processor.selectMicrophone("unplugged"));
    EXPECT_TRUE(processor.selectMicrophone(""));
    EXPECT_EQ(backend->selectedMicrophone, "");
}

TEST(
    ProcessorTest, ReportsMicrophoneChanges
)
{
    auto [backend, processor] = makeFakeProcessor();

    EXPECT_FALSE(processor.haveMicrophonesChanged());
    backend->microphonesChanged = true;
    EXPECT_TRUE(processor.haveMicrophonesChanged());
}

TEST(
    ProcessorTest, TreatsMissingSpeechSupportAsNoMicrophones
)
{
    auto [backend, processor] = makeFakeProcessor();
    backend->microphonesStatus = DataProcessorStatus::UnsupportedFeature;

    EXPECT_TRUE(processor.microphones().empty());
    EXPECT_FALSE(processor.haveMicrophonesChanged());
}

TEST(
    ProcessorTest, DrivesEyeTrackingCalibration
)
{
    auto [backend, processor] = makeFakeProcessor();

    processor.beginEyeTrackingCalibration();
    processor.setEyeTrackingCalibrationTarget(0.1F, 0.9F);
    EXPECT_TRUE(processor.finishEyeTrackingCalibration());
    processor.cancelEyeTrackingCalibration();

    backend->calibrationFinished = false;
    EXPECT_FALSE(processor.finishEyeTrackingCalibration());

    backend->calibrationTargetAccepted = false;
    EXPECT_EQ(
        errorStatus([&processor] -> void {
            processor.setEyeTrackingCalibrationTarget(0.5F, 0.5F);
        }),
        ufeel::Status::InvalidState
    );
}

TEST(
    ProcessorTest, ReadsCalibrationProgressAndDiagnostics
)
{
    auto [backend, processor] = makeFakeProcessor();

    const ufeel::CalibrationProgress progress = processor.eyeTrackingCalibrationProgress();
    const ufeel::CalibrationDiagnostics diagnostics = processor.eyeTrackingCalibrationDiagnostics();

    EXPECT_EQ(progress.targetsWithEnoughSamples, 2);
    EXPECT_EQ(progress.targetCount, 3);
    EXPECT_EQ(progress.samplesForCurrentTarget, 4);
    EXPECT_EQ(progress.requiredSamplesPerTarget, 5);
    EXPECT_FALSE(progress.currentTargetComplete());
    EXPECT_EQ(diagnostics.failureReason, ufeel::CalibrationFailureReason::InsufficientSamples);
    EXPECT_EQ(diagnostics.maximumRetainedSamples, 8);
    EXPECT_FLOAT_EQ(diagnostics.verticalSpan, 0.7F);
    EXPECT_TRUE(diagnostics.meanErrorExceeded);
    EXPECT_FALSE(diagnostics.maximumErrorExceeded);
    EXPECT_EQ(diagnostics.targetsWithInsufficientRetainedSamples, 4);
}

TEST(
    ProcessorTest, SavesAndLoadsCalibrationProfiles
)
{
    auto [backend, processor] = makeFakeProcessor();

    EXPECT_TRUE(processor.saveEyeTrackingCalibrationProfile("profile.txt"));
    EXPECT_TRUE(processor.loadEyeTrackingCalibrationProfile("profile.txt"));

    backend->profileSaved = false;
    backend->profileLoaded = false;
    EXPECT_FALSE(processor.saveEyeTrackingCalibrationProfile("profile.txt"));
    EXPECT_FALSE(processor.loadEyeTrackingCalibrationProfile("profile.txt"));
    EXPECT_THROW(
        static_cast<void>(processor.loadEyeTrackingCalibrationProfile("")), std::invalid_argument
    );
}
