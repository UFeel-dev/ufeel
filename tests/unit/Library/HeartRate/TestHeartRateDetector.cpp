#include "HeartRate/HeartRateDetector.hpp"
#include "HeartRate/UFeelHeartRateStatus.hpp"
#include "Library/HeartRate/TestSupport/HeartRateTestHelpers.hpp"

#include <cstdint>
#include <gtest/gtest.h>

TEST(
    HeartRateDetectorTest, WatchSourceReturnsReceivedReading
)
{
    const std::uint16_t port = reserveUdpPort();
    ASSERT_NE(port, 0U);
    const HeartRateSensorDetector detector(HeartRateSourceType::Watch, port);

    ASSERT_TRUE(sendHeartRatePacket(port, "84", 2));

    const auto result = waitForHeartRate(detector, 84);

    EXPECT_EQ(result.bpm, 84U);
    EXPECT_EQ(result.status, HeartRateStatus::Valid);
}

TEST(
    HeartRateDetectorTest, AutoSourceReturnsAValidReadingWhenWatchPacketArrives
)
{
    const std::uint16_t port = reserveUdpPort();
    ASSERT_NE(port, 0U);
    const HeartRateSensorDetector detector(HeartRateSourceType::Auto, port);

    ASSERT_TRUE(sendHeartRatePacket(port, "96", 2));

    const auto result = waitForValidHeartRate(detector);

    EXPECT_EQ(result.status, HeartRateStatus::Valid);
}

TEST(
    HeartRateDetectorTest, ReturnsNoDataWhenSourceTypeIsUnsupported
)
{
    const HeartRateSensorDetector detector(static_cast<HeartRateSourceType>(255));

    const auto result = detector.process();

    EXPECT_EQ(result.bpm, 0U);
    EXPECT_EQ(result.status, HeartRateStatus::NoData);
}
