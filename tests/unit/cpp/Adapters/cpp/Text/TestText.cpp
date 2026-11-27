#include "ufeel/Model/Calibration.hpp"
#include "ufeel/Model/EmotionResult.hpp"
#include "ufeel/Model/EyeTrackingResult.hpp"
#include "ufeel/Model/Flags.hpp"
#include "ufeel/Model/HeartRateResult.hpp"
#include "ufeel/Model/Microphone.hpp"
#include "ufeel/Model/SpeechToTextResult.hpp"
#include "ufeel/Text/Text.hpp"

#include <gtest/gtest.h>
#include <vector>

namespace
{

auto trackedEyes() -> ufeel::EyeTrackingResult
{
    return {
        .status = ufeel::EyeTrackingStatus::NotCalibrated,
        .directions = ufeel::EyeTrackingDirection::Left | ufeel::EyeTrackingDirection::Up,
        .gazeX = -0.004F,
        .gazeY = 0.25F,
        .leftOpenness = 0.9F,
        .rightOpenness = 0.85F,
        .eyeState = ufeel::EyeState::Tracked,
        .blinkCount = 3,
    };
}

} // namespace

TEST(
    TextTest, ListsEmotionsByScore
)
{
    const ufeel::EmotionResult result{
        .status = ufeel::EmotionStatus::Valid,
        .scores = {{"angry", 0.1F}, {"happy", 0.75F}, {"sad", 0.1F}},
    };

    EXPECT_EQ(ufeel::text::emotions(result), "happy: 0.750\nangry: 0.100\nsad: 0.100");
    EXPECT_EQ(ufeel::text::dominantEmotion(result), "Happy");
}

TEST(
    TextTest, DescribesUnavailableEmotions
)
{
    EXPECT_EQ(
        ufeel::text::emotions({.status = ufeel::EmotionStatus::FaceUnavailable, .scores = {}}),
        "Face not detected"
    );
    EXPECT_EQ(
        ufeel::text::emotions({.status = ufeel::EmotionStatus::InternalError, .scores = {}}),
        "Emotion data unavailable"
    );
    EXPECT_EQ(
        ufeel::text::emotions({.status = ufeel::EmotionStatus::Valid, .scores = {}}), "No data"
    );
    EXPECT_EQ(ufeel::text::dominantEmotion({}), "None");
}

TEST(
    TextTest, DescribesSpeech
)
{
    EXPECT_EQ(ufeel::text::speechToText({}), "Speech recognition is disabled");
    EXPECT_EQ(
        ufeel::text::speechToText({.status = ufeel::SpeechToTextStatus::NoSpeech, .text = {}}),
        "No speech detected"
    );
    EXPECT_EQ(
        ufeel::text::speechToText({.status = ufeel::SpeechToTextStatus::Valid, .text = "bonjour"}),
        "bonjour"
    );
}

TEST(
    TextTest, NamesTheDefaultMicrophone
)
{
    const std::vector<ufeel::Microphone> microphones{
        {.id = "a", .name = "Built-in"},
        {.id = "b", .name = "Headset", .isDefault = true},
    };

    EXPECT_EQ(ufeel::text::defaultMicrophone(microphones), "Default microphone (Headset)");
    EXPECT_EQ(ufeel::text::defaultMicrophone({}), "Default microphone");
}

TEST(
    TextTest, DescribesGazeDirections
)
{
    ufeel::EyeTrackingResult closed = trackedEyes();
    closed.eyeState = closed.eyeState | ufeel::EyeState::LeftClosed | ufeel::EyeState::RightClosed;
    ufeel::EyeTrackingResult still = trackedEyes();
    still.directions = ufeel::EyeTrackingDirection::None;

    EXPECT_EQ(ufeel::text::directions(trackedEyes()), "left, up");
    EXPECT_EQ(ufeel::text::directions(closed), "Eyes closed");
    EXPECT_EQ(ufeel::text::directions(still), "No direction");
    EXPECT_EQ(
        ufeel::text::directions({.status = ufeel::EyeTrackingStatus::LowQuality}),
        "Eye landmarks are low quality"
    );
    EXPECT_EQ(ufeel::text::directions({}), "Camera frame unavailable");
}

TEST(
    TextTest, DescribesTrackedEyes
)
{
    EXPECT_EQ(
        ufeel::text::eyes(trackedEyes()), "Gaze: (+0.00, +0.25)\nOpenness: L 0.90 R 0.85\nBlinks: 3"
    );
    EXPECT_EQ(ufeel::text::eyes({}), "");
}

TEST(
    TextTest, DescribesScreenPositions
)
{
    const ufeel::EyeTrackingResult valid{
        .status = ufeel::EyeTrackingStatus::Valid,
        .estimatedX = 0.5F,
        .estimatedY = 0.25F,
        .recordedX = 0.125F,
        .recordedY = 1.0F,
        .confidence = 0.876F,
    };

    EXPECT_EQ(
        ufeel::text::eyeTracking(valid),
        "Recorded: (0.125, 1.000)\nEstimated: (0.500, 0.250)\nConfidence: 88 %"
    );
    EXPECT_EQ(ufeel::text::eyeTracking({}), "Screen position unavailable");
}

TEST(
    TextTest, DescribesHeartRates
)
{
    EXPECT_EQ(ufeel::text::heartRate({.status = ufeel::HeartRateStatus::Valid, .bpm = 72}), "72");
    EXPECT_EQ(
        ufeel::text::heartRate({.status = ufeel::HeartRateStatus::RequestFailed}), "Request failed"
    );
    EXPECT_EQ(ufeel::text::heartRate({}), "No data");
}

TEST(
    TextTest, DescribesCalibrationFailures
)
{
    const ufeel::CalibrationDiagnostics diagnostics{
        .failureReason = ufeel::CalibrationFailureReason::TooFewValidTargets,
        .inputTargetCount = 9,
        .targetsWithEnoughRawSamples = 7,
        .targetsWithEnoughRetainedSamples = 6,
        .validTargetCount = 5,
        .minimumRetainedSamples = 3,
        .maximumRetainedSamples = 12,
        .horizontalSpan = 0.5F,
        .verticalSpan = 0.25F,
        .meanErrorExceeded = true,
    };

    EXPECT_EQ(
        ufeel::text::calibrationFailure(diagnostics),
        "Failure: Not enough valid calibration targets were collected.\n"
        "Valid targets: 5/9\n"
        "Targets with enough raw samples: 7\n"
        "Targets with enough retained samples: 6\n"
        "Retained samples per target: 3-12\n"
        "Horizontal span: 0.500; vertical span: 0.250\n"
        "The mean-error threshold was exceeded."
    );
    EXPECT_EQ(
        ufeel::text::failureReason(ufeel::CalibrationFailureReason::None),
        "Calibration failed without a specific failure reason."
    );
}
