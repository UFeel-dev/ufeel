#ifndef HEARTRATE_HEARTRATEDETECTOR_HPP
#define HEARTRATE_HEARTRATEDETECTOR_HPP

#include "HeartRate/HeartRateSource.hpp"
#include "HeartRate/SensorHeartRateSource.hpp"
#include "HeartRate/WatchHeartRateSource.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

enum class HeartRateSourceType : std::uint8_t
{
    Auto = 0,
    Sensor = 1,
    Watch = 2,
};

class HeartRateSensorDetector
{
public:

    explicit HeartRateSensorDetector(
        HeartRateSourceType source = HeartRateSourceType::Auto,
        std::uint16_t watchPort = WatchHeartRateSource::kDefaultPort,
        const std::string& sensorUrl = SensorHeartRateSource::kDefaultUrl
    );
    explicit HeartRateSensorDetector(std::vector<std::unique_ptr<HeartRateSource>> sources);

    ~HeartRateSensorDetector() = default;

    HeartRateSensorDetector(const HeartRateSensorDetector&) = delete;
    auto operator=(const HeartRateSensorDetector&) -> HeartRateSensorDetector& = delete;
    HeartRateSensorDetector(HeartRateSensorDetector&&) = delete;
    auto operator=(HeartRateSensorDetector&&) -> HeartRateSensorDetector& = delete;

    [[nodiscard]]
    auto process() const -> HeartRateResult;

private:

    std::vector<std::unique_ptr<HeartRateSource>> sources_;
};

#endif // HEARTRATE_HEARTRATEDETECTOR_HPP
