#include "ufeel.h"

#include <gtest/gtest.h>

TEST(
    HeartRateApiTest, NullProcessorReturnsInvalidProcessor
)
{
    EXPECT_EQ(ufeel_update_heart_rate(nullptr), UFEEL_STATUS_INVALID_PROCESSOR);
    EXPECT_EQ(ufeel_get_heart_rate_result(nullptr, nullptr), UFEEL_STATUS_INVALID_PROCESSOR);
}

TEST(
    HeartRateApiTest, GetterRejectsNullOutput
)
{
    auto* processor = reinterpret_cast<UFeelProcessor*>(1);

    EXPECT_EQ(ufeel_get_heart_rate_result(processor, nullptr), UFEEL_STATUS_INVALID_ARGUMENT);
}

TEST(
    HeartRateApiTest, ResultAllocationAndReleaseAreNullSafe
)
{
    auto* result = ufeel_create_heart_rate_result();

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->status, 0);
    EXPECT_EQ(result->bpm, 0U);
    ufeel_free_heart_rate_result(result);
    EXPECT_NO_THROW(ufeel_free_heart_rate_result(nullptr));
}
