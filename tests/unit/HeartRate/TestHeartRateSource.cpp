#include "HeartRate/HeartRateSource.hpp"
#include "HeartRate/UFeelHeartRateStatus.hpp"

#include <chrono>
#include <gtest/gtest.h>
#include <thread>

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
    LatestReadingTest, ExpiresReadingAfterFiveSeconds
)
{
    LatestReading reading;
    reading.set({.bpm = 72, .status = HeartRateStatus::Valid});

    std::this_thread::sleep_for(std::chrono::milliseconds(5100));

    const auto result = reading.get();

    EXPECT_EQ(result.bpm, 0U);
    EXPECT_EQ(result.status, HeartRateStatus::NoData);
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
