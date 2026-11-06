#include "ufeel.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <gtest/gtest.h>
#include <memory>

namespace
{

auto emotionCallbackStatus() -> int32_t&
{
    static int32_t status = -1;
    return status;
}

auto gazeCallbackStatus() -> int32_t&
{
    static int32_t status = -1;
    return status;
}

void recordEmotionResult(
    UFeelEmotionsResult* result
)
{
    emotionCallbackStatus() = result->status;
}

void recordEyeTrackingResult(
    UFeelEyeTrackingResult* result
)
{
    gazeCallbackStatus() = result->status;
}

struct ProcessorDeleter
{
    void operator()(
        UFeelProcessor* processor
    ) const
    {
        ufeel_destroy(processor);
    }
};

struct EmotionResultDeleter
{
    void operator()(
        UFeelEmotionsResult* result
    ) const
    {
        ufeel_free_emotions_result(result);
    }
};

struct EyeTrackingResultDeleter
{
    void operator()(
        UFeelEyeTrackingResult* result
    ) const
    {
        ufeel_free_eye_tracking_result(result);
    }
};

struct SpeechResultDeleter
{
    void operator()(
        UFeelSpeechToTextResult* result
    ) const
    {
        ufeel_free_speech_to_text_result(result);
    }
};

struct HeartRateResultDeleter
{
    void operator()(
        UFeelHeartRateResult* result
    ) const
    {
        ufeel_free_heart_rate_result(result);
    }
};

} // namespace

TEST(
    WrapperIntegrationTest, ExercisesPublicApiWithOwnedProcessor
)
{
    const std::unique_ptr<UFeelProcessor, ProcessorDeleter> processor(
        ufeel_create(UFEEL_TEST_MODEL_DIR, UFEEL_TEST_SPEECH_MODEL_DIR)
    );
    ASSERT_NE(processor, nullptr);
    const std::unique_ptr<UFeelEmotionsResult, EmotionResultDeleter> emotions(
        ufeel_create_emotions_result()
    );
    ASSERT_NE(emotions, nullptr);
    const std::unique_ptr<UFeelEyeTrackingResult, EyeTrackingResultDeleter> gaze(
        ufeel_create_eye_tracking_result()
    );
    ASSERT_NE(gaze, nullptr);

    constexpr int width = 640;
    constexpr int height = 480;
    constexpr int channels = 3;
    constexpr int stride = width * channels;
    std::array<std::uint8_t, static_cast<std::size_t>(height * stride)> pixels{};
    UFeelFrame input{
        .data = pixels.data(),
        .width = width,
        .height = height,
        .channels = channels,
        .stride = stride,
        .dataSize = pixels.size(),
    };

    UFeelFrame frameInfo{};
    EXPECT_EQ(ufeel_get_frame_info(processor.get(), &frameInfo), UFEEL_STATUS_INVALID_FRAME);
    EXPECT_EQ(ufeel_get_frame(processor.get()), nullptr);
    std::array<std::uint8_t, 4> noFrameDestination{};
    EXPECT_EQ(
        ufeel_copy_frame(processor.get(), noFrameDestination.data(), 4), UFEEL_STATUS_INVALID_FRAME
    );
    EXPECT_EQ(
        ufeel_copy_frame_bgra(processor.get(), noFrameDestination.data(), 4, 1, 1),
        UFEEL_STATUS_INVALID_FRAME
    );
    EXPECT_EQ(ufeel_set_frame(processor.get(), nullptr), UFEEL_STATUS_INVALID_FRAME);
    EXPECT_EQ(
        ufeel_get_emotions_result(processor.get(), emotions.get()), UFEEL_STATUS_INVALID_FRAME
    );
    EXPECT_EQ(
        ufeel_get_eye_tracking_result(processor.get(), gaze.get()), UFEEL_STATUS_NO_EYE_TRACKING
    );
    EXPECT_EQ(
        ufeel_update_emotions_from_frame(processor.get(), nullptr), UFEEL_STATUS_INVALID_FRAME
    );
    emotionCallbackStatus() = -1;
    ufeel_get_emotions_result_from_frame(processor.get(), nullptr, recordEmotionResult);
    EXPECT_EQ(emotionCallbackStatus(), UFEEL_EMOTIONS_STATUS_FRAME_UNAVAILABLE);
    EXPECT_EQ(
        ufeel_update_eye_tracking_from_frame(processor.get(), nullptr), UFEEL_STATUS_INVALID_FRAME
    );
    gazeCallbackStatus() = -1;
    ufeel_get_eye_tracking_result_from_frame(processor.get(), nullptr, recordEyeTrackingResult);
    EXPECT_EQ(gazeCallbackStatus(), UFEEL_GAZE_STATUS_FRAME_UNAVAILABLE);
    EXPECT_EQ(
        ufeel_update_from_frame(processor.get(), 1 << 8, &input), UFEEL_STATUS_INVALID_ARGUMENT
    );
    EXPECT_EQ(ufeel_update_from_frame(processor.get(), 0, nullptr), UFEEL_STATUS_INVALID_FRAME);
    EXPECT_EQ(ufeel_set_frame(processor.get(), &input), UFEEL_STATUS_OK);
    EXPECT_EQ(ufeel_update_from_frame(processor.get(), 0, &input), UFEEL_STATUS_OK);
    EXPECT_EQ(ufeel_update(processor.get(), 0), UFEEL_STATUS_OK);
    std::unique_ptr<UFeelFrame, decltype(&ufeel_free_frame)> copied(
        ufeel_get_frame(processor.get()), &ufeel_free_frame
    );
    ASSERT_NE(copied, nullptr);
    EXPECT_EQ(copied->width, width);
    EXPECT_EQ(copied->height, height);
    EXPECT_EQ(copied->channels, channels);
    EXPECT_EQ(copied->stride, stride);
    EXPECT_EQ(copied->dataSize, pixels.size());

    EXPECT_EQ(ufeel_get_frame_info(processor.get(), &frameInfo), UFEEL_STATUS_OK);
    EXPECT_EQ(frameInfo.width, width);
    EXPECT_EQ(frameInfo.height, height);
    EXPECT_EQ(frameInfo.channels, channels);
    EXPECT_EQ(frameInfo.stride, stride);
    EXPECT_EQ(frameInfo.dataSize, pixels.size());
    EXPECT_EQ(frameInfo.data, nullptr);

    std::array<std::uint8_t, static_cast<std::size_t>(height * stride)> copiedPixels{};
    EXPECT_EQ(
        ufeel_copy_frame(processor.get(), copiedPixels.data(), copiedPixels.size()), UFEEL_STATUS_OK
    );
    EXPECT_EQ(copiedPixels, pixels);
    EXPECT_EQ(
        ufeel_copy_frame(processor.get(), noFrameDestination.data(), 4),
        UFEEL_STATUS_INVALID_ARGUMENT
    );

    std::array<std::uint8_t, static_cast<std::size_t>(height * width * 4)> bgra{};
    EXPECT_EQ(
        ufeel_copy_frame_bgra(processor.get(), bgra.data(), width * 4, width, height),
        UFEEL_STATUS_OK
    );
    EXPECT_EQ(bgra.at(3), 255U);
    EXPECT_EQ(
        ufeel_copy_frame_bgra(processor.get(), bgra.data(), width * 4, width - 1, height),
        UFEEL_STATUS_INVALID_ARGUMENT
    );

    std::array<std::uint8_t, static_cast<std::size_t>(height * width)> grayscale{};
    grayscale.fill(64);
    const UFeelFrame grayFrame{
        .data = grayscale.data(),
        .width = width,
        .height = height,
        .channels = 1,
        .stride = width,
        .dataSize = grayscale.size(),
    };
    EXPECT_EQ(ufeel_set_frame(processor.get(), &grayFrame), UFEEL_STATUS_OK);
    EXPECT_EQ(
        ufeel_copy_frame_bgra(processor.get(), bgra.data(), width * 4, width, height),
        UFEEL_STATUS_OK
    );
    EXPECT_EQ(bgra.at(0), 64U);
    EXPECT_EQ(bgra.at(1), 64U);
    EXPECT_EQ(bgra.at(2), 64U);
    EXPECT_EQ(bgra.at(3), 255U);

    std::array<std::uint8_t, static_cast<std::size_t>(height * width * 4)> fourChannel{};
    fourChannel.fill(24);
    const UFeelFrame fourChannelFrame{
        .data = fourChannel.data(),
        .width = width,
        .height = height,
        .channels = 4,
        .stride = width * 4,
        .dataSize = fourChannel.size(),
    };
    EXPECT_EQ(ufeel_set_frame(processor.get(), &fourChannelFrame), UFEEL_STATUS_OK);
    EXPECT_EQ(
        ufeel_copy_frame_bgra(processor.get(), bgra.data(), width * 4, width, height),
        UFEEL_STATUS_OK
    );
    EXPECT_EQ(bgra.at(0), 24U);
    EXPECT_EQ(bgra.at(3), 24U);

    std::array<std::uint8_t, static_cast<std::size_t>(height * width * 2)> twoChannel{};
    const UFeelFrame twoChannelFrame{
        .data = twoChannel.data(),
        .width = width,
        .height = height,
        .channels = 2,
        .stride = width * 2,
        .dataSize = twoChannel.size(),
    };
    EXPECT_EQ(ufeel_set_frame(processor.get(), &twoChannelFrame), UFEEL_STATUS_OK);
    EXPECT_EQ(
        ufeel_copy_frame_bgra(processor.get(), bgra.data(), width * 4, width, height),
        UFEEL_STATUS_INVALID_FRAME
    );
    EXPECT_EQ(ufeel_set_frame(processor.get(), &input), UFEEL_STATUS_OK);

    const auto emotionUpdateStatus = ufeel_update_emotions_from_frame(processor.get(), &input);
    EXPECT_TRUE(
        emotionUpdateStatus == UFEEL_STATUS_NO_EMOTIONS || emotionUpdateStatus == UFEEL_STATUS_OK
    );
    const auto emotionResultStatus = ufeel_get_emotions_result(processor.get(), emotions.get());
    EXPECT_TRUE(
        emotionResultStatus == UFEEL_STATUS_NO_EMOTIONS || emotionResultStatus == UFEEL_STATUS_OK
    );
    emotionCallbackStatus() = -1;
    ufeel_get_emotions_result_from_frame(processor.get(), &input, recordEmotionResult);
    EXPECT_TRUE(
        emotionCallbackStatus() == UFEEL_EMOTIONS_STATUS_FACE_UNAVAILABLE ||
        emotionCallbackStatus() == UFEEL_EMOTIONS_STATUS_VALID
    );

    EXPECT_EQ(
        ufeel_update_eye_tracking_from_frame(processor.get(), &input), UFEEL_STATUS_NO_EYE_TRACKING
    );
    EXPECT_EQ(
        ufeel_get_eye_tracking_result(processor.get(), gaze.get()), UFEEL_STATUS_NO_EYE_TRACKING
    );
    gazeCallbackStatus() = -1;
    ufeel_get_eye_tracking_result_from_frame(processor.get(), &input, recordEyeTrackingResult);
    EXPECT_EQ(gazeCallbackStatus(), UFEEL_GAZE_STATUS_FACE_UNAVAILABLE);

    UFeelEyeTrackingCalibrationProgress progress{};
    EXPECT_EQ(
        ufeel_get_eye_tracking_calibration_progress(processor.get(), &progress), UFEEL_STATUS_OK
    );
    EXPECT_EQ(ufeel_begin_eye_tracking_calibration(processor.get()), UFEEL_STATUS_OK);
    EXPECT_EQ(
        ufeel_set_eye_tracking_calibration_target(processor.get(), 0.5F, 0.5F), UFEEL_STATUS_OK
    );
    EXPECT_EQ(
        ufeel_set_eye_tracking_calibration_target(processor.get(), 2.0F, 0.5F),
        UFEEL_STATUS_INVALID_STATE
    );
    EXPECT_EQ(
        ufeel_get_eye_tracking_calibration_progress(processor.get(), &progress), UFEEL_STATUS_OK
    );
    EXPECT_EQ(progress.targetCount, 1);
    EXPECT_EQ(
        ufeel_finish_eye_tracking_calibration(processor.get()),
        UFEEL_STATUS_EYE_TRACKING_CALIBRATION_FAILED
    );
    UFeelEyeTrackingCalibrationDiagnostics diagnostics{};
    EXPECT_EQ(
        ufeel_get_eye_tracking_calibration_diagnostics(processor.get(), &diagnostics),
        UFEEL_STATUS_OK
    );
    EXPECT_EQ(
        ufeel_save_eye_tracking_calibration_profile(processor.get(), "/invalid/profile/path"),
        UFEEL_STATUS_EYE_TRACKING_PROFILE_FAILED
    );
    EXPECT_EQ(
        ufeel_load_eye_tracking_calibration_profile(processor.get(), "/invalid/profile/path"),
        UFEEL_STATUS_EYE_TRACKING_PROFILE_FAILED
    );
    EXPECT_EQ(ufeel_cancel_eye_tracking_calibration(processor.get()), UFEEL_STATUS_OK);

    EXPECT_EQ(ufeel_update_heart_rate(processor.get()), UFEEL_STATUS_NO_HEARTRATE);
    const std::unique_ptr<UFeelHeartRateResult, HeartRateResultDeleter> heartRate(
        ufeel_create_heart_rate_result()
    );
    ASSERT_NE(heartRate, nullptr);
    EXPECT_EQ(
        ufeel_get_heart_rate_result(processor.get(), heartRate.get()), UFEEL_STATUS_NO_HEARTRATE
    );

    EXPECT_EQ(ufeel_set_speech_to_text_enabled(processor.get(), 0), UFEEL_STATUS_OK);
    EXPECT_EQ(ufeel_update_speech_to_text(processor.get()), UFEEL_STATUS_NO_SPEECH);
    std::unique_ptr<UFeelSpeechToTextResult, SpeechResultDeleter> speech(
        ufeel_create_speech_to_text_result()
    );
    ASSERT_NE(speech, nullptr);
    EXPECT_EQ(
        ufeel_get_speech_to_text_result(processor.get(), speech.get()), UFEEL_STATUS_NO_SPEECH
    );
    EXPECT_EQ(speech->status, UFEEL_SPEECH_STATUS_DISABLED);

    const auto cameraUpdateStatus = ufeel_update_frame(processor.get());
    EXPECT_TRUE(
        cameraUpdateStatus == UFEEL_STATUS_OK || cameraUpdateStatus == UFEEL_STATUS_INVALID_FRAME
    );
}
