#include "ufeel.h"

#include <cstdint>
#include <gtest/gtest.h>

namespace
{

int32_t callbackStatus = -1;
int32_t callbackCount = -1;

void recordEmotionResult(
    UFeelEmotionsResult* result
)
{
    callbackStatus = result->status;
    callbackCount = result->count;
}

} // namespace

TEST(
    EmotionsApiTest, NullProcessorReturnsInvalidProcessor
)
{
    EXPECT_EQ(ufeel_update_emotions(nullptr), UFEEL_STATUS_INVALID_PROCESSOR);
    EXPECT_EQ(ufeel_update_emotions_from_frame(nullptr, nullptr), UFEEL_STATUS_INVALID_PROCESSOR);
}

TEST(
    EmotionsApiTest, ResultGetterValidatesAndClearsOutput
)
{
    EXPECT_EQ(ufeel_get_emotions_result(nullptr, nullptr), UFEEL_STATUS_INVALID_ARGUMENT);

    UFeelEmotionsResult result{};
    EXPECT_EQ(ufeel_get_emotions_result(nullptr, &result), UFEEL_STATUS_INVALID_PROCESSOR);
    EXPECT_EQ(result.status, 0);
    EXPECT_EQ(result.count, 0);
    EXPECT_EQ(result.emotions, nullptr);
}

TEST(
    EmotionsApiTest, FrameCallbackReportsInvalidProcessor
)
{
    callbackStatus = -1;
    callbackCount = -1;

    ufeel_get_emotions_result_from_frame(nullptr, nullptr, recordEmotionResult);

    EXPECT_EQ(callbackStatus, UFEEL_EMOTIONS_STATUS_INVALID_PROCESSOR);
    EXPECT_EQ(callbackCount, 0);
}

TEST(
    EmotionsApiTest, ResultAllocationAndReleaseAreNullSafe
)
{
    auto* result = ufeel_create_emotions_result();

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->status, 0);
    EXPECT_EQ(result->count, 0);
    EXPECT_EQ(result->emotions, nullptr);
    ufeel_free_emotions_result(result);
    EXPECT_NO_THROW(ufeel_free_emotions_result(nullptr));
    EXPECT_NO_THROW(ufeel_get_emotions_result_from_frame(nullptr, nullptr, nullptr));
}
