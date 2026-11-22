#include "EyeTracking/EyeGazeDirection.hpp"
#include "opencv2/core/types.hpp"

#include <chrono>
#include <gtest/gtest.h>
#include <utility>
#include <vector>

namespace
{

using Clock = EyeGazeDirection::Clock;

constexpr auto FRAME = std::chrono::milliseconds(33);

class Feed
{
public:

    auto operator()(
        EyeGazeSample sample, int frames = 1
    ) -> EyeGazeDirectionOutput
    {
        EyeGazeDirectionOutput output;
        for (int frame = 0; frame < frames; ++frame)
        {
            now += FRAME;
            output = direction.update(sample, now);
        }
        return output;
    }

    EyeGazeDirection direction;
    Clock::time_point now;
};

auto eyes(
    double x, double y
) -> EyeGazeSample
{
    return {.eyeX = x, .eyeY = y};
}

} // namespace

TEST(
    EyeGazeDirectionTest, ReportsDirectionsRelativeToTheLearnedNeutralPose
)
{
    Feed feed;
    auto output = feed(eyes(0.03, 0.02), 60);
    EXPECT_NEAR(output.x, 0.0F, 0.01F);
    EXPECT_EQ(output.horizontal, 0);
    EXPECT_EQ(output.vertical, 0);

    output = feed(eyes(0.03 + EyeGazeDirection::DEFAULT_EYE_RANGE_X, 0.02), 40);
    EXPECT_NEAR(output.x, 1.0F, 0.05F);
    EXPECT_EQ(output.horizontal, 1);

    output = feed(eyes(0.03, 0.02 - EyeGazeDirection::DEFAULT_EYE_RANGE_Y), 40);
    EXPECT_EQ(output.horizontal, 0);
    EXPECT_EQ(output.vertical, -1);
}

TEST(
    EyeGazeDirectionTest, KeepsDirectionUntilGazeFallsBelowTheExitThreshold
)
{
    Feed feed;
    feed(eyes(0.0, 0.0), 120);
    double const range = EyeGazeDirection::DEFAULT_EYE_RANGE_X;

    EXPECT_EQ(feed(eyes(-0.8 * range, 0.0), 30).horizontal, -1);
    EXPECT_EQ(feed(eyes(-0.4 * range, 0.0), 30).horizontal, -1);
    EXPECT_EQ(feed(eyes(-0.1 * range, 0.0), 30).horizontal, 0);
    EXPECT_EQ(feed(eyes(-0.4 * range, 0.0), 30).horizontal, 0);
}

TEST(
    EyeGazeDirectionTest, SmoothsSingleFrameJitter
)
{
    Feed feed;
    feed(eyes(0.0, 0.0), 60);

    auto const output = feed(eyes(EyeGazeDirection::DEFAULT_EYE_RANGE_X, 0.0));
    EXPECT_LT(output.x, 0.5F);
    EXPECT_EQ(output.horizontal, 0);
}

TEST(
    EyeGazeDirectionTest, HeadWeightAddsHeadRotation
)
{
    Feed feed;
    feed({}, 60);
    EyeGazeSample const turned{.headX = EyeGazeDirection::HEAD_RANGE_X};

    EXPECT_EQ(feed(turned, 30).horizontal, 0);

    feed.direction.resetMotion();
    feed.direction.setHeadWeight(1.0);
    EXPECT_EQ(feed(turned, 30).horizontal, 1);

    feed.direction.setHeadWeight(5.0);
    EXPECT_DOUBLE_EQ(feed.direction.headWeight(), 1.0);
}

TEST(
    EyeGazeDirectionTest, FitsRangeFromCalibrationTargets
)
{
    constexpr double neutralX = 0.02;
    constexpr double neutralY = -0.01;
    constexpr double rangeX = 0.025;
    constexpr double rangeY = 0.02;

    std::vector<std::pair<cv::Point2f, std::vector<EyeGazeSample>>> targets;
    for (float const y : {0.1F, 0.5F, 0.9F})
    {
        for (float const x : {0.1F, 0.5F, 0.9F})
        {
            EyeGazeSample const sample =
                eyes(neutralX + ((x - 0.5) * 2.0 * rangeX), neutralY + ((y - 0.5) * 2.0 * rangeY));
            targets.emplace_back(cv::Point2f{x, y}, std::vector<EyeGazeSample>(10, sample));
        }
    }

    auto const range = EyeGazeDirection::fitRange(targets);
    ASSERT_TRUE(range.has_value());
    EXPECT_NEAR(range->neutralX, neutralX, 1.0e-6);
    EXPECT_NEAR(range->neutralY, neutralY, 1.0e-6);
    EXPECT_NEAR(range->rangeX, rangeX, 1.0e-6);
    EXPECT_NEAR(range->rangeY, rangeY, 1.0e-6);

    Feed feed;
    feed.direction.applyRange(range);
    auto const output = feed(eyes(neutralX + rangeX, neutralY), 40);
    EXPECT_NEAR(output.x, 1.0F, 0.05F);
    EXPECT_NEAR(output.y, 0.0F, 0.05F);
}

TEST(
    EyeGazeDirectionTest, RejectsCalibrationWithoutPlausibleEyeMovement
)
{
    std::vector<std::pair<cv::Point2f, std::vector<EyeGazeSample>>> targets;
    for (float const x : {0.1F, 0.9F})
    {
        targets.emplace_back(cv::Point2f{x, x}, std::vector<EyeGazeSample>(10, eyes(0.0, 0.0)));
    }
    EXPECT_FALSE(EyeGazeDirection::fitRange(targets).has_value());

    targets.clear();
    for (float const x : {0.1F, 0.9F})
    {
        targets.emplace_back(cv::Point2f{x, x}, std::vector<EyeGazeSample>(2, eyes(x, x)));
    }
    EXPECT_FALSE(EyeGazeDirection::fitRange(targets).has_value());
}
