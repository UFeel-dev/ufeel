#include "HeartRate/HeartRateSource.hpp"
#include "HeartRate/UFeelHeartRateStatus.hpp"

#include <chrono>
#include <gtest/gtest.h>

TEST(
    LatestReadingTest, ReturnsNoDataBeforeFirstReading
)
{
    const LatestReading reading;

    const auto result = reading.get();

    EXPECT_EQ(result.bpm, 0U);
    EXPECT_EQ(result.status, HeartRateStatus::NoData);
}

TEST(
    LatestReadingTest, ReturnsLatestReadingWhileFresh
)
{
    LatestReading reading;
    reading.set({.bpm = 72, .status = HeartRateStatus::Valid});
    reading.set({.bpm = 88, .status = HeartRateStatus::Valid});

    const auto result = reading.get();

    EXPECT_EQ(result.bpm, 88U);
    EXPECT_EQ(result.status, HeartRateStatus::Valid);
}

TEST(
    LatestReadingTest, KeepsReadingUntilItIsStale
)
{
    const auto now = LatestReading::Clock::now();
    LatestReading reading;
    reading.set({.bpm = 72, .status = HeartRateStatus::Valid}, now);

    const auto fresh = reading.get(now + LatestReading::kStaleAfter);
    const auto stale = reading.get(now + LatestReading::kStaleAfter + std::chrono::milliseconds(1));

    EXPECT_EQ(fresh.bpm, 72U);
    EXPECT_EQ(fresh.status, HeartRateStatus::Valid);
    EXPECT_EQ(stale.bpm, 0U);
    EXPECT_EQ(stale.status, HeartRateStatus::NoData);
}

TEST(
    LatestReadingTest, PreservesNonValidStatusesWhileFresh
)
{
    LatestReading reading;
    reading.set({.bpm = 0, .status = HeartRateStatus::JsonParseError});

    const auto result = reading.get();

    EXPECT_EQ(result.bpm, 0U);
    EXPECT_EQ(result.status, HeartRateStatus::JsonParseError);
}
