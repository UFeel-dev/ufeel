#include "HeartRate/HeartRateDetector.hpp"
#include "HeartRate/HeartRateSource.hpp"
#include "HeartRate/UFeelHeartRateStatus.hpp"
#include "Library/HeartRate/TestSupport/HeartRateSourceFake.hpp"
#include "Library/HeartRate/TestSupport/HeartRateTestHelpers.hpp"
#include "Library/HeartRate/TestSupport/LocalHttpServer.hpp"

#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <utility>
#include <vector>

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
    HeartRateDetectorTest, SensorSourceReadsConfiguredUrl
)
{
    const LocalHttpServer server(200, R"({"bpm": 66})");
    ASSERT_TRUE(server.isReady());
    const HeartRateSensorDetector detector(HeartRateSourceType::Sensor, 0, server.url());

    const auto result = waitForHeartRate(detector, 66);

    EXPECT_EQ(result.bpm, 66U);
    EXPECT_EQ(result.status, HeartRateStatus::Valid);
}

TEST(
    HeartRateDetectorTest, AutoSourceReturnsWatchReadingWhenSensorFails
)
{
    const LocalHttpServer server(503, R"({"error": "unavailable"})");
    ASSERT_TRUE(server.isReady());
    const std::uint16_t port = reserveUdpPort();
    ASSERT_NE(port, 0U);
    const HeartRateSensorDetector detector(HeartRateSourceType::Auto, port, server.url());

    ASSERT_TRUE(sendHeartRatePacket(port, "96", 2));

    const auto result = waitForHeartRate(detector, 96);

    EXPECT_EQ(result.bpm, 96U);
    EXPECT_EQ(result.status, HeartRateStatus::Valid);
}

TEST(
    HeartRateDetectorTest, ReturnsFirstValidInjectedReading
)
{
    std::vector<std::unique_ptr<HeartRateSource>> sources;
    sources.push_back(
        std::make_unique<HeartRateSourceFake>(HeartRateResult{
            .bpm = 0, .status = HeartRateStatus::RequestFailed})
    );
    sources.push_back(
        std::make_unique<HeartRateSourceFake>(HeartRateResult{
            .bpm = 70, .status = HeartRateStatus::Valid})
    );
    sources.push_back(
        std::make_unique<HeartRateSourceFake>(HeartRateResult{
            .bpm = 80, .status = HeartRateStatus::Valid})
    );
    const HeartRateSensorDetector detector(std::move(sources));

    const auto result = detector.process();

    EXPECT_EQ(result.bpm, 70U);
    EXPECT_EQ(result.status, HeartRateStatus::Valid);
}

TEST(
    HeartRateDetectorTest, SingleSourceKeepsItsErrorStatus
)
{
    std::vector<std::unique_ptr<HeartRateSource>> sources;
    sources.push_back(
        std::make_unique<HeartRateSourceFake>(HeartRateResult{
            .bpm = 0, .status = HeartRateStatus::JsonParseError})
    );
    const HeartRateSensorDetector detector(std::move(sources));

    EXPECT_EQ(detector.process().status, HeartRateStatus::JsonParseError);
}

TEST(
    HeartRateDetectorTest, ReturnsNoDataWhenNoSourceIsValid
)
{
    std::vector<std::unique_ptr<HeartRateSource>> sources;
    sources.push_back(
        std::make_unique<HeartRateSourceFake>(HeartRateResult{
            .bpm = 0, .status = HeartRateStatus::RequestFailed})
    );
    sources.push_back(
        std::make_unique<HeartRateSourceFake>(HeartRateResult{
            .bpm = 0, .status = HeartRateStatus::NoData})
    );
    const HeartRateSensorDetector detector(std::move(sources));

    const auto result = detector.process();

    EXPECT_EQ(result.bpm, 0U);
    EXPECT_EQ(result.status, HeartRateStatus::NoData);
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
