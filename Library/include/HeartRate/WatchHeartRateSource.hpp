#ifndef HEARTRATE_WATCHHEARTRATESOURCE_HPP
#define HEARTRATE_WATCHHEARTRATESOURCE_HPP

#include "HeartRate/HeartRateSource.hpp"

#include <cstdint>
#include <stop_token>
#include <thread>

class WatchHeartRateSource final : public HeartRateSource
{
public:

    static constexpr std::uint16_t kDefaultPort = 9999;

    explicit WatchHeartRateSource(std::uint16_t port = kDefaultPort);

    WatchHeartRateSource(const WatchHeartRateSource&) = delete;
    WatchHeartRateSource(WatchHeartRateSource&&) = delete;
    auto operator=(const WatchHeartRateSource&) -> WatchHeartRateSource& = delete;
    auto operator=(WatchHeartRateSource&&) -> WatchHeartRateSource& = delete;
    ~WatchHeartRateSource() override;

    [[nodiscard]]
    auto read() const -> HeartRateResult override;

private:

    void run(const std::stop_token& stop);

    int socket_{-1};
    LatestReading latest_;
    std::jthread worker_;
};

#endif // HEARTRATE_WATCHHEARTRATESOURCE_HPP
