#include "ufeel/Model/EyeTrackingResult.hpp"
#include "ufeel/Pipeline/Clock.hpp"
#include "ufeel/Pipeline/EyeTrackingSmoother.hpp"

#include <chrono>
#include <cmath>
#include <gtest/gtest.h>

namespace
{

constexpr auto timeConstant = std::chrono::milliseconds(80);

auto estimate(
    float x, float y
) -> ufeel::EyeTrackingResult
{
    return {.status = ufeel::EyeTrackingStatus::Valid, .estimatedX = x, .estimatedY = y};
}

} // namespace

TEST(
    EyeTrackingSmootherTest, PassesTheFirstEstimateThrough
)
{
    ufeel::EyeTrackingSmoother smoother;

    const auto smoothed = smoother.smooth(estimate(0.2F, 0.8F), ufeel::Clock::time_point{});

    EXPECT_FLOAT_EQ(smoothed.estimatedX, 0.2F);
    EXPECT_FLOAT_EQ(smoothed.estimatedY, 0.8F);
}

TEST(
    EyeTrackingSmootherTest, MovesTowardsNewEstimatesOverTime
)
{
    ufeel::EyeTrackingSmoother smoother;
    const ufeel::Clock::time_point start{};
    static_cast<void>(smoother.smooth(estimate(0.0F, 1.0F), start));

    const auto smoothed = smoother.smooth(estimate(1.0F, 0.0F), start + timeConstant);

    const auto alpha = static_cast<float>(1.0 - std::exp(-1.0));
    EXPECT_NEAR(smoothed.estimatedX, alpha, 1e-5F);
    EXPECT_NEAR(smoothed.estimatedY, 1.0F - alpha, 1e-5F);
}

TEST(
    EyeTrackingSmootherTest, RestartsAfterInvalidEstimatesAndLongGaps
)
{
    ufeel::EyeTrackingSmoother smoother;
    const ufeel::Clock::time_point start{};
    static_cast<void>(smoother.smooth(estimate(0.0F, 0.0F), start));

    const ufeel::EyeTrackingResult invalid{.status = ufeel::EyeTrackingStatus::FaceUnavailable};
    EXPECT_EQ(smoother.smooth(invalid, start + timeConstant).status, invalid.status);
    EXPECT_FLOAT_EQ(smoother.smooth(estimate(1.0F, 1.0F), start + timeConstant).estimatedX, 1.0F);

    const auto afterGap = start + timeConstant + std::chrono::seconds(1);
    EXPECT_FLOAT_EQ(smoother.smooth(estimate(0.5F, 0.5F), afterGap).estimatedX, 0.5F);
    EXPECT_FLOAT_EQ(smoother.smooth(estimate(0.0F, 0.0F), afterGap).estimatedX, 0.0F);
}
