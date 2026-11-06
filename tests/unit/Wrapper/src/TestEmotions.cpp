#include "ufeel.h"

#include <cstdint>
#include <cstdlib>
#include <gtest/gtest.h>
#include <memory>

namespace
{

auto callbackStatus() -> int32_t&
{
    static int32_t status = -1;
    return status;
}

auto callbackCount() -> int32_t&
{
    static int32_t count = -1;
    return count;
}

void recordEmotionResult(
    UFeelEmotionsResult* result
)
{
    callbackStatus() = result->status;
    callbackCount() = result->count;
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
    EmotionsApiTest, ResultGetterReleasesExistingEmotionPayload
)
{
    const std::unique_ptr<UFeelEmotionsResult, decltype(&ufeel_free_emotions_result)> result(
        ufeel_create_emotions_result(), &ufeel_free_emotions_result
    );
    ASSERT_NE(result, nullptr);
    result->emotions = new UFeelPair[2]{};
    result->count = 2;
    for (int32_t index = 0; index < result->count; ++index)
    {
        result->emotions[index].name = static_cast<char*>(std::malloc(2));
        ASSERT_NE(result->emotions[index].name, nullptr);
    }

    EXPECT_EQ(ufeel_get_emotions_result(nullptr, result.get()), UFEEL_STATUS_INVALID_PROCESSOR);
    EXPECT_EQ(result->count, 0);
    EXPECT_EQ(result->emotions, nullptr);
}

TEST(
    EmotionsApiTest, FrameCallbackReportsInvalidProcessor
)
{
    callbackStatus() = -1;
    callbackCount() = -1;

    ufeel_get_emotions_result_from_frame(nullptr, nullptr, recordEmotionResult);

    EXPECT_EQ(callbackStatus(), UFEEL_EMOTIONS_STATUS_INVALID_PROCESSOR);
    EXPECT_EQ(callbackCount(), 0);
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
