#include "DataProcessor.hpp"
#include "EyeTracking/EyeTrackingDetector.hpp"
#include "HeartRate/UFeelHeartRateStatus.hpp"
#include "ProcessorBackendFake.hpp"
#include "internal/UFeelProcessor.hpp"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "ufeel.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <gtest/gtest.h>
#include <memory>
#include <new>
#include <string>
#include <utility>

namespace
{

class TestWrapperAllocator final : public ufeel::internal::WrapperAllocator
{
public:

    bool failFrame = false;
    bool failFrameData = false;
    bool failEmotionPairs = false;
    int32_t failStringAt = -1;
    int32_t stringAllocations = 0;

    [[nodiscard]] auto createFrame() noexcept -> UFeelFrame* override
    {
        return failFrame ? nullptr : new (std::nothrow) UFeelFrame{};
    }

    [[nodiscard]] auto allocateFrameData(
        std::size_t size
    ) noexcept -> std::uint8_t* override
    {
        return failFrameData ? nullptr : static_cast<std::uint8_t*>(std::malloc(size));
    }

    [[nodiscard]] auto allocateEmotionPairs(
        std::size_t count
    ) noexcept -> UFeelPair* override
    {
        return failEmotionPairs ? nullptr : new (std::nothrow) UFeelPair[count]{};
    }

    [[nodiscard]] auto duplicateString(
        const std::string& value
    ) noexcept -> char* override
    {
        const auto allocation = stringAllocations++;
        if (allocation == failStringAt)
        {
            return nullptr;
        }

        auto* result = static_cast<char*>(std::malloc(value.size() + 1));
        if (result != nullptr)
        {
            std::memcpy(result, value.c_str(), value.size() + 1);
        }
        return result;
    }
};

struct TestProcessor
{
    std::unique_ptr<ProcessorBackendFake> ownedBackend = std::make_unique<ProcessorBackendFake>();
    ProcessorBackendFake* backend = ownedBackend.get();
    TestWrapperAllocator allocator;
    UFeelProcessor processor{std::move(ownedBackend), allocator};
};

struct EmotionCallbackValue
{
    int32_t status = -1;
    int32_t count = -1;
    std::string firstName;
};

auto emotionCallbackValue() -> EmotionCallbackValue&
{
    static EmotionCallbackValue value;
    return value;
}

auto eyeTrackingCallbackValue() -> UFeelEyeTrackingResult&
{
    static UFeelEyeTrackingResult result{};
    return result;
}

void recordEmotionResult(
    UFeelEmotionsResult* result
)
{
    auto& value = emotionCallbackValue();
    value.status = result->status;
    value.count = result->count;
    value.firstName = result->count > 0 ? result->emotions[0].name : "";
}

void recordEyeTrackingResult(
    UFeelEyeTrackingResult* result
)
{
    eyeTrackingCallbackValue() = *result;
}

auto makeFrame(
    std::array<std::uint8_t, 12>& pixels
) -> UFeelFrame
{
    return {
        .data = pixels.data(),
        .width = 2,
        .height = 2,
        .channels = 3,
        .stride = 6,
        .dataSize = pixels.size(),
    };
}

} // namespace

TEST(
    WrapperBehaviorTest, ProcessorUpdateAndFrameApisMapBackendStatuses
)
{
    TestProcessor test;
    std::array<std::uint8_t, 12> pixels{};
    auto frame = makeFrame(pixels);

    test.backend->updateStatus = DataProcessorStatus::InvalidFrame;
    EXPECT_EQ(ufeel_update(&test.processor, 0), UFEEL_STATUS_INVALID_FRAME);
    test.backend->updateStatus = DataProcessorStatus::InvalidDetectionType;
    EXPECT_EQ(ufeel_update(&test.processor, 0), UFEEL_STATUS_INVALID_ARGUMENT);
    test.backend->updateStatus = DataProcessorStatus::Ok;
    EXPECT_EQ(ufeel_update(&test.processor, 0), UFEEL_STATUS_OK);
    test.backend->throwOnUpdate = true;
    EXPECT_EQ(ufeel_update(&test.processor, 0), UFEEL_STATUS_INTERNAL_ERROR);
    test.backend->throwOnUpdate = false;
    test.backend->throwUnknownOnUpdate = true;
    EXPECT_EQ(ufeel_update(&test.processor, 0), UFEEL_STATUS_INTERNAL_ERROR);
    test.backend->throwUnknownOnUpdate = false;
    test.backend->updateStatus = static_cast<DataProcessorStatus>(255);
    EXPECT_EQ(ufeel_update(&test.processor, 0), UFEEL_STATUS_INTERNAL_ERROR);

    test.backend->updateFrameStatus = DataProcessorStatus::FrameUnavailable;
    EXPECT_EQ(ufeel_update_frame(&test.processor), UFEEL_STATUS_INVALID_FRAME);
    test.backend->setFrameStatus = DataProcessorStatus::InvalidFrame;
    EXPECT_EQ(ufeel_set_frame(&test.processor, &frame), UFEEL_STATUS_INVALID_FRAME);
    test.backend->setFrameStatus = DataProcessorStatus::Ok;
    EXPECT_EQ(ufeel_set_frame(&test.processor, &frame), UFEEL_STATUS_OK);

    test.backend->updateStatus = DataProcessorStatus::InvalidDetectionType;
    EXPECT_EQ(ufeel_update_from_frame(&test.processor, 0, &frame), UFEEL_STATUS_INVALID_ARGUMENT);
    EXPECT_EQ(ufeel_update_from_frame(&test.processor, 0, nullptr), UFEEL_STATUS_INVALID_FRAME);
    test.backend->updateStatus = DataProcessorStatus::FrameUnavailable;
    EXPECT_EQ(ufeel_update_from_frame(&test.processor, 0, &frame), UFEEL_STATUS_INVALID_FRAME);
    test.backend->updateStatus = DataProcessorStatus::Ok;
    test.backend->updateFrameStatus = DataProcessorStatus::Ok;
    EXPECT_EQ(ufeel_update_frame(&test.processor), UFEEL_STATUS_OK);

    std::unique_ptr<UFeelFrame, decltype(&ufeel_free_frame)> copy(
        ufeel_get_frame(&test.processor), &ufeel_free_frame
    );
    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->dataSize, pixels.size());
    EXPECT_EQ(copy->width, 2);

    UFeelFrame info{};
    EXPECT_EQ(ufeel_get_frame_info(&test.processor, &info), UFEEL_STATUS_OK);
    EXPECT_EQ(info.stride, 6);
    EXPECT_EQ(info.dataSize, pixels.size());
    EXPECT_EQ(info.data, nullptr);

    std::array<std::uint8_t, 12> output{};
    EXPECT_EQ(ufeel_copy_frame(&test.processor, output.data(), 12), UFEEL_STATUS_OK);
    EXPECT_EQ(output, pixels);
    EXPECT_EQ(ufeel_copy_frame(&test.processor, output.data(), 11), UFEEL_STATUS_INVALID_ARGUMENT);
}

TEST(
    WrapperBehaviorTest, FrameResultReportsAllocationFailures
)
{
    TestProcessor test;
    test.backend->frame = cv::Mat(2, 2, CV_8UC3);

    test.allocator.failFrame = true;
    EXPECT_EQ(ufeel_get_frame(&test.processor), nullptr);

    test.allocator.failFrame = false;
    test.allocator.failFrameData = true;
    EXPECT_EQ(ufeel_get_frame(&test.processor), nullptr);
}

TEST(
    WrapperBehaviorTest, ConvertsBackendExceptionsIntoSafeApiResults
)
{
    TestProcessor test;
    std::array<std::uint8_t, 12> pixels{};
    auto frame = makeFrame(pixels);

    test.backend->throwOnFrame = true;
    EXPECT_EQ(ufeel_get_frame(&test.processor), nullptr);
    UFeelFrame frameInfo{};
    EXPECT_EQ(ufeel_get_frame_info(&test.processor, &frameInfo), UFEEL_STATUS_INTERNAL_ERROR);
    EXPECT_EQ(ufeel_copy_frame(&test.processor, pixels.data(), 12), UFEEL_STATUS_INTERNAL_ERROR);
    EXPECT_EQ(
        ufeel_copy_frame_bgra(&test.processor, pixels.data(), 8, 2, 2), UFEEL_STATUS_INTERNAL_ERROR
    );
    test.backend->throwOnFrame = false;

    test.backend->throwOnEmotion = true;
    EXPECT_EQ(ufeel_update_emotions(&test.processor), UFEEL_STATUS_INTERNAL_ERROR);
    EXPECT_EQ(
        ufeel_update_emotions_from_frame(&test.processor, &frame), UFEEL_STATUS_INTERNAL_ERROR
    );
    UFeelEmotionsResult emotions{};
    EXPECT_EQ(ufeel_get_emotions_result(&test.processor, &emotions), UFEEL_STATUS_INTERNAL_ERROR);
    emotionCallbackValue() = {};
    ufeel_get_emotions_result_from_frame(&test.processor, &frame, recordEmotionResult);
    EXPECT_EQ(emotionCallbackValue().status, -1);
    test.backend->throwOnEmotion = false;

    test.backend->throwOnEyeTracking = true;
    EXPECT_EQ(ufeel_update_eye_tracking(&test.processor), UFEEL_STATUS_INTERNAL_ERROR);
    EXPECT_EQ(
        ufeel_update_eye_tracking_from_frame(&test.processor, &frame), UFEEL_STATUS_INTERNAL_ERROR
    );
    UFeelEyeTrackingResult eyeTracking{};
    EXPECT_EQ(
        ufeel_get_eye_tracking_result(&test.processor, &eyeTracking), UFEEL_STATUS_INTERNAL_ERROR
    );
    eyeTrackingCallbackValue() = {};
    ufeel_get_eye_tracking_result_from_frame(&test.processor, &frame, recordEyeTrackingResult);
    EXPECT_EQ(eyeTrackingCallbackValue().status, 0);
    test.backend->throwOnEyeTracking = false;

    test.backend->throwOnSpeech = true;
    EXPECT_EQ(ufeel_update_speech_to_text(&test.processor), UFEEL_STATUS_INTERNAL_ERROR);
    auto* speech = ufeel_create_speech_to_text_result();
    ASSERT_NE(speech, nullptr);
    EXPECT_EQ(
        ufeel_get_speech_to_text_result(&test.processor, speech), UFEEL_STATUS_INTERNAL_ERROR
    );
    EXPECT_EQ(speech->speech, nullptr);
    ufeel_free_speech_to_text_result(speech);
    test.backend->throwOnSpeech = false;

    test.backend->throwOnHeartRate = true;
    EXPECT_EQ(ufeel_update_heart_rate(&test.processor), UFEEL_STATUS_INTERNAL_ERROR);
    UFeelHeartRateResult heartRate{};
    EXPECT_EQ(
        ufeel_get_heart_rate_result(&test.processor, &heartRate), UFEEL_STATUS_INTERNAL_ERROR
    );
    test.backend->throwOnHeartRate = false;

    test.backend->throwOnCalibration = true;
    EXPECT_EQ(ufeel_begin_eye_tracking_calibration(&test.processor), UFEEL_STATUS_INTERNAL_ERROR);
    EXPECT_EQ(
        ufeel_set_eye_tracking_calibration_target(&test.processor, 0.1F, 0.1F),
        UFEEL_STATUS_INTERNAL_ERROR
    );
    EXPECT_EQ(ufeel_finish_eye_tracking_calibration(&test.processor), UFEEL_STATUS_INTERNAL_ERROR);
    EXPECT_EQ(ufeel_cancel_eye_tracking_calibration(&test.processor), UFEEL_STATUS_INTERNAL_ERROR);
    UFeelEyeTrackingCalibrationProgress progress{};
    EXPECT_EQ(
        ufeel_get_eye_tracking_calibration_progress(&test.processor, &progress),
        UFEEL_STATUS_INTERNAL_ERROR
    );
    UFeelEyeTrackingCalibrationDiagnostics diagnostics{};
    EXPECT_EQ(
        ufeel_get_eye_tracking_calibration_diagnostics(&test.processor, &diagnostics),
        UFEEL_STATUS_INTERNAL_ERROR
    );
    EXPECT_EQ(
        ufeel_save_eye_tracking_calibration_profile(&test.processor, "profile"),
        UFEEL_STATUS_INTERNAL_ERROR
    );
    EXPECT_EQ(
        ufeel_load_eye_tracking_calibration_profile(&test.processor, "profile"),
        UFEEL_STATUS_INTERNAL_ERROR
    );
}

TEST(
    WrapperBehaviorTest, FrameCopyValidatesLayoutAndConvertsSupportedChannelCounts
)
{
    TestProcessor test;
    std::array<std::uint8_t, 4> pixels{17, 34, 51, 68};
    std::array<std::uint8_t, 16> output{};
    UFeelFrame input{
        .data = pixels.data(),
        .width = 2,
        .height = 2,
        .channels = 1,
        .stride = 2,
        .dataSize = pixels.size(),
    };

    ASSERT_EQ(ufeel_set_frame(&test.processor, &input), UFEEL_STATUS_OK);
    EXPECT_EQ(ufeel_copy_frame_bgra(&test.processor, output.data(), 8, 2, 2), UFEEL_STATUS_OK);
    EXPECT_EQ(output.at(0), 17U);
    EXPECT_EQ(output.at(3), 255U);
    EXPECT_EQ(
        ufeel_copy_frame_bgra(&test.processor, output.data(), 8, 1, 2),
        UFEEL_STATUS_INVALID_ARGUMENT
    );

    std::array<std::uint8_t, 12> rgb{};
    input = {
        .data = rgb.data(),
        .width = 2,
        .height = 2,
        .channels = 3,
        .stride = 6,
        .dataSize = rgb.size(),
    };
    ASSERT_EQ(ufeel_set_frame(&test.processor, &input), UFEEL_STATUS_OK);
    EXPECT_EQ(ufeel_copy_frame_bgra(&test.processor, output.data(), 8, 2, 2), UFEEL_STATUS_OK);

    std::array<std::uint8_t, 16> rgba{};
    input = {
        .data = rgba.data(),
        .width = 2,
        .height = 2,
        .channels = 4,
        .stride = 8,
        .dataSize = rgba.size(),
    };
    ASSERT_EQ(ufeel_set_frame(&test.processor, &input), UFEEL_STATUS_OK);
    EXPECT_EQ(ufeel_copy_frame_bgra(&test.processor, output.data(), 8, 2, 2), UFEEL_STATUS_OK);

    std::array<std::uint8_t, 8> twoChannel{};
    input.data = twoChannel.data();
    input.channels = 2;
    input.stride = 4;
    input.dataSize = twoChannel.size();
    ASSERT_EQ(ufeel_set_frame(&test.processor, &input), UFEEL_STATUS_OK);
    EXPECT_EQ(
        ufeel_copy_frame_bgra(&test.processor, output.data(), 8, 2, 2), UFEEL_STATUS_INVALID_FRAME
    );
    EXPECT_EQ(
        ufeel_copy_frame_bgra(&test.processor, output.data(), 7, 2, 2),
        UFEEL_STATUS_INVALID_ARGUMENT
    );

    test.backend->frame = cv::Mat(2, 2, CV_16UC1);
    EXPECT_EQ(
        ufeel_copy_frame_bgra(&test.processor, output.data(), 8, 2, 2), UFEEL_STATUS_INVALID_FRAME
    );
}

TEST(
    WrapperBehaviorTest, EmotionApisConvertResultsAndReportEveryOutcome
)
{
    TestProcessor test;
    std::array<std::uint8_t, 12> pixels{};
    auto frame = makeFrame(pixels);

    test.backend->emotionStatus = DataProcessorStatus::InvalidFrame;
    EXPECT_EQ(ufeel_update_emotions(&test.processor), UFEEL_STATUS_INVALID_FRAME);
    EXPECT_EQ(
        ufeel_update_emotions_from_frame(&test.processor, &frame), UFEEL_STATUS_INVALID_FRAME
    );
    test.backend->emotionStatus = DataProcessorStatus::Ok;
    EXPECT_EQ(ufeel_update_emotions(&test.processor), UFEEL_STATUS_OK);
    EXPECT_EQ(ufeel_update_emotions_from_frame(&test.processor, &frame), UFEEL_STATUS_OK);
    test.backend->emotions.status = EmotionStatus::FrameUnavailable;
    EXPECT_EQ(ufeel_update_emotions(&test.processor), UFEEL_STATUS_NO_EMOTIONS);

    auto* result = ufeel_create_emotions_result();
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(ufeel_get_emotions_result(&test.processor, result), UFEEL_STATUS_INVALID_FRAME);
    EXPECT_EQ(result->status, static_cast<int32_t>(EmotionStatus::FrameUnavailable));

    test.backend->emotions = {.status = EmotionStatus::Valid, .emotions = {{"joy", 0.75F}}};
    EXPECT_EQ(ufeel_get_emotions_result(&test.processor, result), UFEEL_STATUS_OK);
    ASSERT_EQ(result->count, 1);
    EXPECT_STREQ(result->emotions[0].name, "joy");
    EXPECT_FLOAT_EQ(result->emotions[0].value, 0.75F);

    test.backend->emotions = {.status = EmotionStatus::FaceUnavailable};
    EXPECT_EQ(ufeel_get_emotions_result(&test.processor, result), UFEEL_STATUS_NO_EMOTIONS);
    EXPECT_EQ(result->count, 0);
    EXPECT_EQ(result->emotions, nullptr);
    test.backend->emotions = {.status = EmotionStatus::Valid};
    EXPECT_EQ(ufeel_get_emotions_result(&test.processor, result), UFEEL_STATUS_OK);
    EXPECT_EQ(result->count, 0);
    EXPECT_EQ(result->emotions, nullptr);
    ufeel_free_emotions_result(result);

    emotionCallbackValue() = {};
    ufeel_get_emotions_result_from_frame(&test.processor, nullptr, recordEmotionResult);
    EXPECT_EQ(emotionCallbackValue().status, UFEEL_EMOTIONS_STATUS_FRAME_UNAVAILABLE);

    test.backend->frameEmotions = {
        .status = EmotionStatus::Valid,
        .emotions = {{"calm", 0.4F}, {"joy", 0.8F}},
    };
    ufeel_get_emotions_result_from_frame(&test.processor, &frame, recordEmotionResult);
    EXPECT_EQ(emotionCallbackValue().status, UFEEL_EMOTIONS_STATUS_VALID);
    EXPECT_EQ(emotionCallbackValue().count, 2);
    EXPECT_EQ(emotionCallbackValue().firstName, "calm");
}

TEST(
    WrapperBehaviorTest, EmotionResultReportsPairAndStringAllocationFailures
)
{
    TestProcessor test;
    test.backend->emotions = {
        .status = EmotionStatus::Valid,
        .emotions = {{"calm", 0.4F}, {"joy", 0.8F}},
    };
    auto* result = ufeel_create_emotions_result();
    ASSERT_NE(result, nullptr);

    test.allocator.failEmotionPairs = true;
    EXPECT_EQ(ufeel_get_emotions_result(&test.processor, result), UFEEL_STATUS_INTERNAL_ERROR);
    EXPECT_EQ(result->emotions, nullptr);
    EXPECT_EQ(result->count, 0);

    std::array<std::uint8_t, 12> pixels{};
    auto frame = makeFrame(pixels);
    test.backend->frameEmotions = test.backend->emotions;
    emotionCallbackValue() = {};
    ufeel_get_emotions_result_from_frame(&test.processor, &frame, recordEmotionResult);
    EXPECT_EQ(emotionCallbackValue().status, UFEEL_EMOTIONS_STATUS_INTERNAL_ERROR);

    test.allocator.failEmotionPairs = false;
    test.allocator.failStringAt = 0;
    test.allocator.stringAllocations = 0;
    EXPECT_EQ(ufeel_get_emotions_result(&test.processor, result), UFEEL_STATUS_INTERNAL_ERROR);
    EXPECT_EQ(result->emotions, nullptr);
    EXPECT_EQ(result->count, 0);

    test.allocator.failStringAt = 1;
    test.allocator.stringAllocations = 0;
    EXPECT_EQ(ufeel_get_emotions_result(&test.processor, result), UFEEL_STATUS_INTERNAL_ERROR);
    EXPECT_EQ(result->emotions, nullptr);
    EXPECT_EQ(result->count, 0);
    ufeel_free_emotions_result(result);
}

TEST(
    WrapperBehaviorTest, EyeTrackingApisMapCalibrationResultsAndCopyDiagnostics
)
{
    TestProcessor test;
    std::array<std::uint8_t, 12> pixels{};
    auto frame = makeFrame(pixels);

    test.backend->eyeTrackingStatus = DataProcessorStatus::InvalidFrame;
    EXPECT_EQ(ufeel_update_eye_tracking(&test.processor), UFEEL_STATUS_INVALID_FRAME);
    EXPECT_EQ(
        ufeel_update_eye_tracking_from_frame(&test.processor, &frame), UFEEL_STATUS_INVALID_FRAME
    );
    test.backend->eyeTrackingStatus = DataProcessorStatus::Ok;
    test.backend->eyeTracking.status = EyeTrackingStatus::Valid;
    EXPECT_EQ(ufeel_update_eye_tracking(&test.processor), UFEEL_STATUS_OK);
    test.backend->eyeTracking.status = EyeTrackingStatus::FaceUnavailable;
    EXPECT_EQ(ufeel_update_eye_tracking(&test.processor), UFEEL_STATUS_NO_EYE_TRACKING);

    UFeelEyeTrackingResult result{};
    EXPECT_EQ(
        ufeel_get_eye_tracking_result(&test.processor, &result), UFEEL_STATUS_NO_EYE_TRACKING
    );
    EXPECT_EQ(result.status, static_cast<int32_t>(EyeTrackingStatus::FaceUnavailable));
    result.x = 0.3F;
    test.backend->eyeTracking = {
        .x = 0.2F,
        .y = 0.7F,
        .recordedX = 0.3F,
        .recordedY = 0.6F,
        .confidence = 0.9F,
        .directions = EyeTrackingLeft | EyeTrackingUp,
        .status = EyeTrackingStatus::Valid,
    };
    EXPECT_EQ(ufeel_get_eye_tracking_result(&test.processor, &result), UFEEL_STATUS_OK);
    EXPECT_FLOAT_EQ(result.x, 0.2F);
    EXPECT_FLOAT_EQ(result.recordedY, 0.6F);
    EXPECT_EQ(result.directions, EyeTrackingLeft | EyeTrackingUp);

    eyeTrackingCallbackValue() = {};
    ufeel_get_eye_tracking_result_from_frame(&test.processor, nullptr, recordEyeTrackingResult);
    EXPECT_EQ(eyeTrackingCallbackValue().status, UFEEL_GAZE_STATUS_FRAME_UNAVAILABLE);
    ufeel_get_eye_tracking_result_from_frame(&test.processor, &frame, recordEyeTrackingResult);
    EXPECT_EQ(eyeTrackingCallbackValue().status, UFEEL_GAZE_STATUS_VALID);
    EXPECT_FLOAT_EQ(eyeTrackingCallbackValue().x, 0.2F);

    EXPECT_EQ(ufeel_begin_eye_tracking_calibration(&test.processor), UFEEL_STATUS_OK);
    test.backend->calibrationTargetAccepted = false;
    EXPECT_EQ(
        ufeel_set_eye_tracking_calibration_target(&test.processor, 0.2F, 0.4F),
        UFEEL_STATUS_INVALID_STATE
    );
    test.backend->calibrationTargetAccepted = true;
    EXPECT_EQ(
        ufeel_set_eye_tracking_calibration_target(&test.processor, 0.2F, 0.4F), UFEEL_STATUS_OK
    );
    test.backend->calibrationFinished = false;
    EXPECT_EQ(
        ufeel_finish_eye_tracking_calibration(&test.processor),
        UFEEL_STATUS_EYE_TRACKING_CALIBRATION_FAILED
    );
    test.backend->calibrationFinished = true;
    EXPECT_EQ(ufeel_finish_eye_tracking_calibration(&test.processor), UFEEL_STATUS_OK);
    EXPECT_EQ(ufeel_cancel_eye_tracking_calibration(&test.processor), UFEEL_STATUS_OK);

    UFeelEyeTrackingCalibrationProgress progress{};
    EXPECT_EQ(
        ufeel_get_eye_tracking_calibration_progress(&test.processor, &progress), UFEEL_STATUS_OK
    );
    EXPECT_EQ(progress.samplesForCurrentTarget, 4);
    UFeelEyeTrackingCalibrationDiagnostics diagnostics{};
    EXPECT_EQ(
        ufeel_get_eye_tracking_calibration_diagnostics(&test.processor, &diagnostics),
        UFEEL_STATUS_OK
    );
    EXPECT_EQ(diagnostics.invalidTargetCount, 2);
    EXPECT_EQ(diagnostics.meanErrorExceeded, 1);

    EXPECT_EQ(
        ufeel_save_eye_tracking_calibration_profile(&test.processor, "profile"), UFEEL_STATUS_OK
    );
    test.backend->profileSaved = false;
    EXPECT_EQ(
        ufeel_save_eye_tracking_calibration_profile(&test.processor, "profile"),
        UFEEL_STATUS_EYE_TRACKING_PROFILE_FAILED
    );
    EXPECT_EQ(
        ufeel_load_eye_tracking_calibration_profile(&test.processor, "profile"), UFEEL_STATUS_OK
    );
    test.backend->profileLoaded = false;
    EXPECT_EQ(
        ufeel_load_eye_tracking_calibration_profile(&test.processor, "profile"),
        UFEEL_STATUS_EYE_TRACKING_PROFILE_FAILED
    );
}

TEST(
    WrapperBehaviorTest, SpeechAndHeartRateApisReturnBackendStatusesAndValues
)
{
    TestProcessor test;

    test.backend->speechStatus = DataProcessorStatus::InvalidFrame;
    EXPECT_EQ(ufeel_update_speech_to_text(&test.processor), UFEEL_STATUS_INVALID_FRAME);
    test.backend->speechStatus = DataProcessorStatus::Ok;
    EXPECT_EQ(ufeel_update_speech_to_text(&test.processor), UFEEL_STATUS_OK);
    test.backend->speech.status = SpeechToTextStatus::NoSpeech;
    EXPECT_EQ(ufeel_update_speech_to_text(&test.processor), UFEEL_STATUS_NO_SPEECH);
    EXPECT_EQ(ufeel_set_speech_to_text_enabled(&test.processor, -1), UFEEL_STATUS_OK);
    EXPECT_TRUE(test.backend->speechEnabled);
    EXPECT_EQ(ufeel_set_speech_to_text_enabled(&test.processor, 0), UFEEL_STATUS_OK);
    EXPECT_FALSE(test.backend->speechEnabled);

    auto* speech = ufeel_create_speech_to_text_result();
    ASSERT_NE(speech, nullptr);
    EXPECT_EQ(ufeel_get_speech_to_text_result(&test.processor, speech), UFEEL_STATUS_NO_SPEECH);
    EXPECT_STREQ(speech->speech, "recognized");
    test.backend->speech = {.status = SpeechToTextStatus::Valid, .text = ""};
    EXPECT_EQ(ufeel_get_speech_to_text_result(&test.processor, speech), UFEEL_STATUS_OK);
    ASSERT_NE(speech->speech, nullptr);
    EXPECT_STREQ(speech->speech, "");
    ufeel_free_speech_to_text_result(speech);

    test.backend->heartRateStatus = DataProcessorStatus::InvalidFrame;
    EXPECT_EQ(ufeel_update_heart_rate(&test.processor), UFEEL_STATUS_INVALID_FRAME);
    test.backend->heartRateStatus = DataProcessorStatus::Ok;
    test.backend->heartRate.status = HeartRateStatus::Valid;
    EXPECT_EQ(ufeel_update_heart_rate(&test.processor), UFEEL_STATUS_OK);
    test.backend->heartRate.status = HeartRateStatus::NoData;
    EXPECT_EQ(ufeel_update_heart_rate(&test.processor), UFEEL_STATUS_NO_HEARTRATE);

    UFeelHeartRateResult heartRate{};
    EXPECT_EQ(ufeel_get_heart_rate_result(&test.processor, &heartRate), UFEEL_STATUS_NO_HEARTRATE);
    EXPECT_EQ(heartRate.status, static_cast<int32_t>(HeartRateStatus::NoData));
    test.backend->heartRate = {.bpm = 73, .status = HeartRateStatus::Valid};
    EXPECT_EQ(ufeel_get_heart_rate_result(&test.processor, &heartRate), UFEEL_STATUS_OK);
    EXPECT_EQ(heartRate.bpm, 73U);
}

TEST(
    WrapperBehaviorTest, SpeechResultReportsStringAllocationFailure
)
{
    TestProcessor test;
    auto* speech = ufeel_create_speech_to_text_result();
    ASSERT_NE(speech, nullptr);

    test.allocator.failStringAt = 0;
    EXPECT_EQ(
        ufeel_get_speech_to_text_result(&test.processor, speech), UFEEL_STATUS_INTERNAL_ERROR
    );
    EXPECT_EQ(speech->speech, nullptr);
    ufeel_free_speech_to_text_result(speech);
}
