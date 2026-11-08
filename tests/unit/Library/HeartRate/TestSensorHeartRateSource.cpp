#include "HeartRate/SensorHeartRateSource.hpp"
#include "HeartRate/UFeelHeartRateStatus.hpp"
#include "Library/HeartRate/TestSupport/HeartRateTestHelpers.hpp"
#include "Library/HeartRate/TestSupport/LocalHttpServer.hpp"

#include <gtest/gtest.h>

TEST(
    SensorHeartRateSourceTest, ReadsBpmFromSuccessfulJsonResponse
)
{
    const LocalHttpServer server(200, R"({"bpm": 72})");
    ASSERT_TRUE(server.isReady());
    const SensorHeartRateSource source(server.url());

    const auto result = waitForHeartRate(source, HeartRateStatus::Valid, 72);

    EXPECT_EQ(result.bpm, 72U);
    EXPECT_EQ(result.status, HeartRateStatus::Valid);
}

TEST(
    SensorHeartRateSourceTest, ReturnsRequestFailedForHttpError
)
{
    const LocalHttpServer server(503, R"({"error": "unavailable"})");
    ASSERT_TRUE(server.isReady());
    const SensorHeartRateSource source(server.url());

    const auto result = waitForHeartRate(source, HeartRateStatus::RequestFailed);

    EXPECT_EQ(result.bpm, 0U);
    EXPECT_EQ(result.status, HeartRateStatus::RequestFailed);
}

TEST(
    SensorHeartRateSourceTest, ReturnsRequestFailedWhenServerCannotBeReached
)
{
    const SensorHeartRateSource source("http://127.0.0.1:1");

    const auto result = waitForHeartRate(source, HeartRateStatus::RequestFailed);

    EXPECT_EQ(result.bpm, 0U);
    EXPECT_EQ(result.status, HeartRateStatus::RequestFailed);
}

TEST(
    SensorHeartRateSourceTest, ReturnsJsonParseErrorForMalformedJson
)
{
    const LocalHttpServer server(200, R"({"bpm": )");
    ASSERT_TRUE(server.isReady());
    const SensorHeartRateSource source(server.url());

    const auto result = waitForHeartRate(source, HeartRateStatus::JsonParseError);

    EXPECT_EQ(result.bpm, 0U);
    EXPECT_EQ(result.status, HeartRateStatus::JsonParseError);
}

TEST(
    SensorHeartRateSourceTest, ReturnsJsonParseOutOfRangeWhenBpmIsMissing
)
{
    const LocalHttpServer server(200, R"({"heart_rate": 72})");
    ASSERT_TRUE(server.isReady());
    const SensorHeartRateSource source(server.url());

    const auto result = waitForHeartRate(source, HeartRateStatus::JsonParseOutOfRange);

    EXPECT_EQ(result.bpm, 0U);
    EXPECT_EQ(result.status, HeartRateStatus::JsonParseOutOfRange);
}

TEST(
    SensorHeartRateSourceTest, ReturnsJsonParseErrorWhenBpmHasWrongType
)
{
    const LocalHttpServer server(200, R"({"bpm": "72"})");
    ASSERT_TRUE(server.isReady());
    const SensorHeartRateSource source(server.url());

    const auto result = waitForHeartRate(source, HeartRateStatus::JsonParseError);

    EXPECT_EQ(result.bpm, 0U);
    EXPECT_EQ(result.status, HeartRateStatus::JsonParseError);
}
