#include "HeartRate/HeartRateDetector.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace
{

class LocalHttpServer
{
public:

    LocalHttpServer(
        int statusCode, std::string body
    )
        : statusCode_(statusCode)
        , body_(std::move(body))
    {
        serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);

        if (serverSocket_ < 0)
        {
            return;
        }

        int reuseAddress = 1;
        setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &reuseAddress, sizeof(reuseAddress));

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = htons(8000);

        if (bind(serverSocket_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0 ||
            listen(serverSocket_, 1) < 0)
        {
            close(serverSocket_);
            serverSocket_ = -1;
            return;
        }

        worker_ = std::thread([this] {
            const int client = accept(serverSocket_, nullptr, nullptr);
            if (client < 0)
            {
                return;
            }

            char request[4096];
            static_cast<void>(recv(client, request, sizeof(request), 0));

            const std::string response = "HTTP/1.1 " + std::to_string(statusCode_) +
                                         " Test\r\n"
                                         "Content-Type: application/json\r\n"
                                         "Content-Length: " +
                                         std::to_string(body_.size()) +
                                         "\r\n"
                                         "Connection: close\r\n\r\n" +
                                         body_;

            static_cast<void>(send(client, response.data(), response.size(), 0));

            close(client);
        });
    }

    ~LocalHttpServer()
    {
        if (serverSocket_ >= 0)
        {
            shutdown(serverSocket_, SHUT_RDWR);
            close(serverSocket_);
        }

        if (worker_.joinable())
        {
            worker_.join();
        }
    }

    LocalHttpServer(const LocalHttpServer&) = delete;
    auto operator=(const LocalHttpServer&) -> LocalHttpServer& = delete;

    [[nodiscard]]
    auto isReady() const -> bool
    {
        return serverSocket_ >= 0;
    }

private:

    int statusCode_;
    std::string body_;
    int serverSocket_{-1};
    std::thread worker_;
};

TEST(
    HeartRateDetectorTest, ProcessReturnsValidBpm
)
{
    LocalHttpServer server(200, R"({"bpm": 72})");
    ASSERT_TRUE(server.isReady());

    const HeartRateResult result = HeartRateSensorDetector::process();

    EXPECT_EQ(result.bpm, 72U);
    EXPECT_EQ(result.status, HeartRateStatus::Valid);
}

TEST(
    HeartRateDetectorTest, ProcessReturnsRequestFailedForHttpError
)
{
    LocalHttpServer server(500, R"({"error": "internal server error"})");
    ASSERT_TRUE(server.isReady());

    const HeartRateResult result = HeartRateSensorDetector::process();

    EXPECT_EQ(result.bpm, 0U);
    EXPECT_EQ(result.status, HeartRateStatus::RequestFailed);
}

TEST(
    HeartRateDetectorTest, ProcessReturnsJsonParseErrorForMalformedJson
)
{
    LocalHttpServer server(200, R"({"bpm": )");
    ASSERT_TRUE(server.isReady());

    const HeartRateResult result = HeartRateSensorDetector::process();

    EXPECT_EQ(result.bpm, 0U);
    EXPECT_EQ(result.status, HeartRateStatus::JsonParseError);
}

TEST(
    HeartRateDetectorTest, ProcessReturnsJsonParseOutOfRangeForMissingBpm
)
{
    LocalHttpServer server(200, R"({"heart_rate": 72})");
    ASSERT_TRUE(server.isReady());

    const HeartRateResult result = HeartRateSensorDetector::process();

    EXPECT_EQ(result.bpm, 0U);
    EXPECT_EQ(result.status, HeartRateStatus::JsonParseOutOfRange);
}

} // namespace
