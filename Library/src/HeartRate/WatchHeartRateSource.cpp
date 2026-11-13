#include "HeartRate/WatchHeartRateSource.hpp"

#include "HeartRate/HeartRateSource.hpp"
#include "HeartRate/UFeelHeartRateStatus.hpp"

#include <array>
#include <asm-generic/socket.h>
#include <charconv>
#include <cstdint>
#include <iostream>
#include <netinet/in.h>
#include <poll.h>
#include <stop_token>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <system_error>
#include <thread>
#include <unistd.h>

namespace
{

constexpr std::uint32_t kMinBpm = 20;
constexpr std::uint32_t kMaxBpm = 250;
constexpr int kPollTimeoutMs = 200;

} // namespace

WatchHeartRateSource::WatchHeartRateSource(
    std::uint16_t port
)
{
    socket_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_ < 0)
    {
        std::cerr << "[ERROR] HeartRate: cannot create UDP socket\n";
        return;
    }

    int const reuse = 1;
    ::setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (::bind(socket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
    {
        std::cerr << "[ERROR] HeartRate: cannot bind UDP port " << port << "\n";
        ::close(socket_);
        socket_ = -1;
        return;
    }

    socklen_t addressLength = sizeof(addr);
    if (::getsockname(socket_, reinterpret_cast<sockaddr*>(&addr), &addressLength) == 0)
    {
        port_ = ntohs(addr.sin_port);
    }

    worker_ = std::jthread([this](const std::stop_token& stop) -> void { run(stop); });
}

WatchHeartRateSource::~WatchHeartRateSource()
{
    worker_.request_stop();
    if (worker_.joinable())
    {
        worker_.join();
    }

    if (socket_ >= 0)
    {
        ::close(socket_);
    }
}

auto WatchHeartRateSource::read() const -> HeartRateResult
{
    return latest_.get();
}

auto WatchHeartRateSource::port() const -> std::uint16_t
{
    return port_;
}

void WatchHeartRateSource::run(
    const std::stop_token& stop
)
{
    std::array<char, 32> buffer{};

    while (!stop.stop_requested())
    {
        pollfd pfd{
            .fd = socket_,
            .events = POLLIN,
            .revents = 0,
        };

        if (::poll(&pfd, 1, kPollTimeoutMs) <= 0)
        {
            continue;
        }

        ssize_t const size = ::recv(socket_, buffer.data(), buffer.size(), 0);
        if (size <= 0)
        {
            continue;
        }

        std::uint32_t bpm = 0;
        auto const [ptr, ec] = std::from_chars(buffer.data(), buffer.data() + size, bpm);

        if (ec != std::errc{} || bpm < kMinBpm || bpm > kMaxBpm)
        {
            continue;
        }

        latest_.set({
            .bpm = bpm,
            .status = HeartRateStatus::Valid,
        });
    }
}
