#include "ufeel.h"

#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <memory>

#ifndef UFEEL_TEST_MODEL_DIR
#define UFEEL_TEST_MODEL_DIR ""
#endif
#ifndef UFEEL_TEST_SPEECH_MODEL_DIR
#define UFEEL_TEST_SPEECH_MODEL_DIR ""
#endif

TEST(
    ProcessorApiTest, CreateRejectsMissingPaths
)
{
    EXPECT_EQ(ufeel_create(nullptr, "speech"), nullptr);
    EXPECT_EQ(ufeel_create("", "speech"), nullptr);
    EXPECT_EQ(ufeel_create("models", nullptr), nullptr);
    EXPECT_EQ(ufeel_create("models", ""), nullptr);
}

TEST(
    ProcessorApiTest, CreateReturnsNullWhenModelConfigurationCannotBeLoaded
)
{
    EXPECT_EQ(ufeel_create("missing-model-directory", "missing-speech-model"), nullptr);
}

TEST(
    ProcessorApiTest, UpdatesImageFeaturesThroughProductionBackend
)
{
    std::unique_ptr<UFeelProcessor, decltype(&ufeel_destroy)> processor(
        ufeel_create(UFEEL_TEST_MODEL_DIR, UFEEL_TEST_SPEECH_MODEL_DIR), ufeel_destroy
    );
    ASSERT_NE(processor, nullptr);

    const auto emotionStatus = ufeel_update_emotions(processor.get());
    EXPECT_TRUE(emotionStatus == UFEEL_STATUS_OK || emotionStatus == UFEEL_STATUS_INVALID_FRAME);

    const auto eyeTrackingStatus = ufeel_update_eye_tracking(processor.get());
    EXPECT_TRUE(
        eyeTrackingStatus == UFEEL_STATUS_OK || eyeTrackingStatus == UFEEL_STATUS_INVALID_FRAME ||
        eyeTrackingStatus == UFEEL_STATUS_NO_EYE_TRACKING
    );
}

TEST(
    ProcessorApiTest, NullProcessorOperationsReturnSafeResults
)
{
    EXPECT_EQ(ufeel_update(nullptr, 0), UFEEL_STATUS_INVALID_PROCESSOR);
    EXPECT_EQ(ufeel_update_from_frame(nullptr, 0, nullptr), UFEEL_STATUS_INVALID_PROCESSOR);
    EXPECT_EQ(ufeel_update_frame(nullptr), UFEEL_STATUS_INVALID_PROCESSOR);
    EXPECT_EQ(ufeel_set_frame(nullptr, nullptr), UFEEL_STATUS_INVALID_PROCESSOR);
    EXPECT_EQ(ufeel_get_frame(nullptr), nullptr);
    EXPECT_EQ(ufeel_get_frame_info(nullptr, nullptr), UFEEL_STATUS_INVALID_PROCESSOR);
    EXPECT_EQ(ufeel_copy_frame(nullptr, nullptr, 0), UFEEL_STATUS_INVALID_PROCESSOR);
    EXPECT_EQ(ufeel_copy_frame_bgra(nullptr, nullptr, 0, 0, 0), UFEEL_STATUS_INVALID_PROCESSOR);
}

TEST(
    ProcessorApiTest, FrameReleaseAcceptsNull
)
{
    EXPECT_NO_THROW(ufeel_free_frame(nullptr));
}

TEST(
    ProcessorApiTest, FrameInfoValidatesOutput
)
{
    auto* processor = reinterpret_cast<UFeelProcessor*>(1);

    EXPECT_EQ(ufeel_get_frame_info(processor, nullptr), UFEEL_STATUS_INVALID_ARGUMENT);
}

TEST(
    ProcessorApiTest, RejectsInvalidFrameCopyArguments
)
{
    auto* processor = reinterpret_cast<UFeelProcessor*>(1);
    std::array<std::uint8_t, 4> destination{};

    EXPECT_EQ(ufeel_copy_frame(processor, nullptr, 4), UFEEL_STATUS_INVALID_ARGUMENT);
    EXPECT_EQ(ufeel_copy_frame(processor, destination.data(), 0), UFEEL_STATUS_INVALID_ARGUMENT);
    EXPECT_EQ(ufeel_copy_frame_bgra(processor, nullptr, 4, 1, 1), UFEEL_STATUS_INVALID_ARGUMENT);
    EXPECT_EQ(
        ufeel_copy_frame_bgra(processor, destination.data(), 3, 1, 1), UFEEL_STATUS_INVALID_ARGUMENT
    );
    EXPECT_EQ(
        ufeel_copy_frame_bgra(processor, destination.data(), 4, 0, 1), UFEEL_STATUS_INVALID_ARGUMENT
    );
}
