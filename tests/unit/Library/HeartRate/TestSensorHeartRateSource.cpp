#include "HeartRate/SensorHeartRateSource.hpp"
#include "HeartRate/UFeelHeartRateStatus.hpp"
#include "TestSupport/HeartRateTestHelpers.hpp"
#include "TestSupport/LocalHttpServer.hpp"

#include <gtest/gtest.h>

TEST(
    SensorHeartRateSourceTest, ReadsBpmFromSuccessfulJsonResponse
)
{
    LocalHttpServer server(200, R"({"bpm": 72})");
    ASSERT_TRUE(server.isReady());
    SensorHeartRateSource source(server.url());

    const auto result = waitForHeartRate(source, HeartRateStatus::Valid, 72);

    EXPECT_EQ(result.bpm, 72U);
    EXPECT_EQ(result.status, HeartRateStatus::Valid);
}

TEST(
    SensorHeartRateSourceTest, ReturnsRequestFailedForHttpError
)
{
    LocalHttpServer server(503, R"({"error": "unavailable"})");
    ASSERT_TRUE(server.isReady());
    SensorHeartRateSource source(server.url());

    const auto result = waitForHeartRate(source, HeartRateStatus::RequestFailed);

    EXPECT_EQ(result.bpm, 0U);
    EXPECT_EQ(result.status, HeartRateStatus::RequestFailed);
}

TEST(
    SensorHeartRateSourceTest, ReturnsRequestFailedWhenServerCannotBeReached
)
{
    SensorHeartRateSource source("http://127.0.0.1:1");

    const auto result = waitForHeartRate(source, HeartRateStatus::RequestFailed);

    EXPECT_EQ(result.bpm, 0U);
    EXPECT_EQ(result.status, HeartRateStatus::RequestFailed);
}

TEST(
    SensorHeartRateSourceTest, ReturnsJsonParseErrorForMalformedJson
)
{
    LocalHttpServer server(200, R"({"bpm": )");
    ASSERT_TRUE(server.isReady());
    SensorHeartRateSource source(server.url());

    const auto result = waitForHeartRate(source, HeartRateStatus::JsonParseError);

    EXPECT_EQ(result.bpm, 0U);
    EXPECT_EQ(result.status, HeartRateStatus::JsonParseError);
}

TEST(
    SensorHeartRateSourceTest, ReturnsJsonParseOutOfRangeWhenBpmIsMissing
)
{
    LocalHttpServer server(200, R"({"heart_rate": 72})");
    ASSERT_TRUE(server.isReady());
    SensorHeartRateSource source(server.url());

    const auto result = waitForHeartRate(source, HeartRateStatus::JsonParseOutOfRange);

    EXPECT_EQ(result.bpm, 0U);
    EXPECT_EQ(result.status, HeartRateStatus::JsonParseOutOfRange);
}

TEST(
    SensorHeartRateSourceTest, ReturnsJsonParseErrorWhenBpmHasWrongType
)
{
    LocalHttpServer server(200, R"({"bpm": "72"})");
    ASSERT_TRUE(server.isReady());
    SensorHeartRateSource source(server.url());

    const auto result = waitForHeartRate(source, HeartRateStatus::JsonParseError);

    EXPECT_EQ(result.bpm, 0U);
    EXPECT_EQ(result.status, HeartRateStatus::JsonParseError);
}
