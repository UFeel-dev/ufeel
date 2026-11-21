#include "EyeTracking/EyeBlinkTracker.hpp"

#include <chrono>
#include <gtest/gtest.h>
#include <limits>

namespace
{

using Clock = EyeBlinkTracker::Clock;

constexpr double OPEN = 0.25;
constexpr double CLOSED = 0.05;
constexpr auto FRAME = std::chrono::milliseconds(33);

class Feed
{
public:

    auto operator()(
        double left, double right, int frames = 1
    ) -> EyeBlinkState
    {
        EyeBlinkState state;
        for (int frame = 0; frame < frames; ++frame)
        {
            now += FRAME;
            state = tracker.update(left, right, now);
        }
        return state;
    }

    EyeBlinkTracker tracker;
    Clock::time_point now;
};

} // namespace

TEST(
    EyeBlinkTrackerTest, CountsBlinkWhenBothEyesReopen
)
{
    Feed feed;
    auto state = feed(OPEN, OPEN, 30);
    EXPECT_FALSE(state.leftClosed || state.rightClosed);
    EXPECT_NEAR(state.leftOpenness, 1.0F, 0.01F);

    state = feed(CLOSED, CLOSED, 4);
    EXPECT_TRUE(state.leftClosed && state.rightClosed);
    EXPECT_GT(state.eyesClosedSeconds, 0.0F);
    EXPECT_EQ(state.blinkCount, 0U);

    state = feed(OPEN, OPEN);
    EXPECT_EQ(state.blinkCount, 1U);
    EXPECT_FLOAT_EQ(state.eyesClosedSeconds, 0.0F);
}

TEST(
    EyeBlinkTrackerTest, IgnoresNoiseAroundTheCloseThreshold
)
{
    Feed feed;
    feed(OPEN, OPEN, 30);

    feed(CLOSED, CLOSED);
    for (int frame = 0; frame < 10; ++frame)
    {
        double const hovering = OPEN * (frame % 2 == 0 ? 0.5 : 0.55);
        EXPECT_TRUE(feed(hovering, hovering).leftClosed);
    }
    EXPECT_EQ(feed(OPEN, OPEN).blinkCount, 1U);
}

TEST(
    EyeBlinkTrackerTest, LoweredLidsAreNotABlink
)
{
    Feed feed;
    feed(OPEN, OPEN, 30);

    auto const state = feed(OPEN * 0.6, OPEN * 0.6, 60);
    EXPECT_FALSE(state.leftClosed || state.rightClosed);
    EXPECT_EQ(feed(OPEN, OPEN).blinkCount, 0U);
}

TEST(
    EyeBlinkTrackerTest, ReportsWinkWithoutCountingABlink
)
{
    Feed feed;
    feed(OPEN, OPEN, 30);

    auto const state = feed(CLOSED, OPEN, 5);
    EXPECT_TRUE(state.leftClosed);
    EXPECT_FALSE(state.rightClosed);
    EXPECT_FLOAT_EQ(state.eyesClosedSeconds, 0.0F);
    EXPECT_EQ(feed(OPEN, OPEN).blinkCount, 0U);
}

TEST(
    EyeBlinkTrackerTest, LongClosureIsNotABlink
)
{
    Feed feed;
    feed(OPEN, OPEN, 30);

    auto const state = feed(CLOSED, CLOSED, 60);
    EXPECT_GT(state.eyesClosedSeconds, 1.5F);
    EXPECT_EQ(feed(OPEN, OPEN).blinkCount, 0U);
}

TEST(
    EyeBlinkTrackerTest, InterruptedClosureIsNotCounted
)
{
    Feed feed;
    feed(OPEN, OPEN, 30);
    feed(CLOSED, CLOSED, 3);

    feed.tracker.interrupt();
    EXPECT_EQ(feed(OPEN, OPEN).blinkCount, 0U);

    feed(CLOSED, CLOSED, 3);
    auto const state = feed(std::numeric_limits<double>::quiet_NaN(), OPEN);
    EXPECT_FALSE(state.leftClosed || state.rightClosed);
    EXPECT_EQ(feed(OPEN, OPEN).blinkCount, 0U);
}

TEST(
    EyeBlinkTrackerTest, DetectsClosedEyesOnFirstFrame
)
{
    Feed feed;
    auto const state = feed(CLOSED, CLOSED);
    EXPECT_TRUE(state.leftClosed && state.rightClosed);
}
