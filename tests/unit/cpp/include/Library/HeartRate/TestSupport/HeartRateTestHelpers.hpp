#ifndef LIBRARY_HEARTRATE_TESTSUPPORT_HEARTRATETESTHELPERS_HPP
#define LIBRARY_HEARTRATE_TESTSUPPORT_HEARTRATETESTHELPERS_HPP

#include "HeartRate/HeartRateDetector.hpp"
#include "HeartRate/HeartRateSource.hpp"
#include "HeartRate/UFeelHeartRateStatus.hpp"

#include <arpa/inet.h>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <utility>

inline auto waitForHeartRate(
    const HeartRateSource& source, HeartRateStatus status, std::uint32_t bpm = 0
) -> HeartRateResult
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);

    while (std::chrono::steady_clock::now() < deadline)
    {
        const auto result = source.read();
        if (result.status == status && result.bpm == bpm)
        {
            return result;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return source.read();
}

inline auto waitForHeartRate(
    const HeartRateSensorDetector& detector, std::uint32_t bpm
) -> HeartRateResult
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);

    while (std::chrono::steady_clock::now() < deadline)
    {
        const auto result = detector.process();
        if (result.status == HeartRateStatus::Valid && result.bpm == bpm)
        {
            return result;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return detector.process();
}

inline auto reserveUdpPort() -> std::uint16_t
{
    const int socket = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (socket < 0)
    {
        return 0;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = 0;

    if (::bind(socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0)
    {
        static_cast<void>(::close(socket));
        return 0;
    }

    socklen_t addressLength = sizeof(address);
    if (::getsockname(socket, reinterpret_cast<sockaddr*>(&address), &addressLength) < 0)
    {
        static_cast<void>(::close(socket));
        return 0;
    }

    const auto port = ntohs(address.sin_port);
    static_cast<void>(::close(socket));
    return port;
}

inline auto sendHeartRatePacket(
    std::uint16_t port, const char* data, std::size_t size
) -> bool
{
    const int socket = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (socket < 0)
    {
        return false;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(port);

    const ssize_t sent =
        ::sendto(socket, data, size, 0, reinterpret_cast<sockaddr*>(&address), sizeof(address));
    static_cast<void>(::close(socket));
    return std::cmp_equal(sent, size);
}

#endif // LIBRARY_HEARTRATE_TESTSUPPORT_HEARTRATETESTHELPERS_HPP
