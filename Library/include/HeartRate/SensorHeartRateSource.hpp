#ifndef HEARTRATE_SENSORHEARTRATESOURCE_HPP
#define HEARTRATE_SENSORHEARTRATESOURCE_HPP

#include "HeartRate/HeartRateSource.hpp"

#include <stop_token>
#include <string>
#include <thread>

class SensorHeartRateSource final : public HeartRateSource
{
public:

    static constexpr const char* kDefaultUrl = "http://localhost:8000";

    explicit SensorHeartRateSource(std::string url = kDefaultUrl);

    [[nodiscard]]
    auto read() const -> HeartRateResult override;

private:

    [[nodiscard]]
    auto poll() const -> HeartRateResult;

    void run(const std::stop_token& stop);

    std::string url_;
    LatestReading latest_;
    std::jthread worker_;
};

#endif // HEARTRATE_SENSORHEARTRATESOURCE_HPP
