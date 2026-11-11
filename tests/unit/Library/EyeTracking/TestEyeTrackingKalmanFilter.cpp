#include "EyeTracking/EyeTrackingKalmanFilter.hpp"
#include "opencv2/core/types.hpp"

#include <chrono>
#include <gtest/gtest.h>
#include <limits>

namespace
{

using Clock = std::chrono::steady_clock;

} // namespace

TEST(
    EyeTrackingKalmanFilterTest, InitializesWithClampedFiniteMeasurements
)
{
    EyeTrackingKalmanFilter filter;
    const auto now = Clock::time_point{};

    EXPECT_EQ(filter.update({0.25F, 0.75F}, 1.0, now), cv::Point2f(0.25F, 0.75F));
    filter.reset();
    EXPECT_EQ(filter.update({-1.0F, 2.0F}, 1.0, now), cv::Point2f(0.0F, 1.0F));
}

TEST(
    EyeTrackingKalmanFilterTest, IgnoresNonFiniteMeasurements
)
{
    EyeTrackingKalmanFilter filter;
    const auto now = Clock::time_point{};
    const float nan = std::numeric_limits<float>::quiet_NaN();

    EXPECT_EQ(filter.update({nan, 0.5F}, 1.0, now), cv::Point2f{});
    EXPECT_EQ(filter.update({0.4F, 0.6F}, 1.0, now), cv::Point2f(0.4F, 0.6F));
    EXPECT_EQ(
        filter.update({0.4F, nan}, 1.0, now + std::chrono::milliseconds(20)),
        cv::Point2f(0.4F, 0.6F)
    );
}

TEST(
    EyeTrackingKalmanFilterTest, HandlesInvalidTimeStepsByReinitializing
)
{
    EyeTrackingKalmanFilter filter;
    const auto now = Clock::time_point{};
    filter.update({0.2F, 0.3F}, 1.0, now);

    EXPECT_EQ(filter.update({0.7F, 0.8F}, 1.0, now), cv::Point2f(0.7F, 0.8F));
    EXPECT_EQ(
        filter.update({0.1F, 0.9F}, 1.0, now + std::chrono::seconds(1)), cv::Point2f(0.1F, 0.9F)
    );
}

TEST(
    EyeTrackingKalmanFilterTest, RejectsOutliersThenReinitializesAfterRepeatedRejections
)
{
    EyeTrackingKalmanFilter filter;
    const auto now = Clock::time_point{};
    filter.update({0.0F, 0.0F}, 1.0, now);

    const auto firstOutlier = filter.update({1.0F, 1.0F}, 1.0, now + std::chrono::milliseconds(20));
    EXPECT_NEAR(firstOutlier.x, 0.0F, 0.01F);
    EXPECT_NEAR(firstOutlier.y, 0.0F, 0.01F);

    EXPECT_EQ(
        filter.update({1.0F, 1.0F}, 1.0, now + std::chrono::milliseconds(40)),
        cv::Point2f(1.0F, 1.0F)
    );
}

TEST(
    EyeTrackingKalmanFilterTest, TreatsNonFiniteQualityAsLowConfidence
)
{
    EyeTrackingKalmanFilter filter;
    const auto now = Clock::time_point{};
    filter.update({0.5F, 0.5F}, 1.0, now);

    const auto position = filter.update(
        {0.6F, 0.6F}, std::numeric_limits<double>::quiet_NaN(), now + std::chrono::milliseconds(20)
    );

    EXPECT_GT(position.x, 0.5F);
    EXPECT_GT(position.y, 0.5F);
}
