#include "HeartRate/HeartRateDetector.hpp"

#include "HeartRate/HeartRateSource.hpp"
#include "HeartRate/SensorHeartRateSource.hpp"
#include "HeartRate/UFeelHeartRateStatus.hpp"
#include "HeartRate/WatchHeartRateSource.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

HeartRateSensorDetector::HeartRateSensorDetector(
    HeartRateSourceType source, std::uint16_t watchPort, const std::string& sensorUrl
)
{
    if (source == HeartRateSourceType::Auto || source == HeartRateSourceType::Sensor)
    {
        sources_.push_back(std::make_unique<SensorHeartRateSource>(sensorUrl));
    }

    if (source == HeartRateSourceType::Auto || source == HeartRateSourceType::Watch)
    {
        sources_.push_back(std::make_unique<WatchHeartRateSource>(watchPort));
    }
}

HeartRateSensorDetector::HeartRateSensorDetector(
    std::vector<std::unique_ptr<HeartRateSource>> sources
)
    : sources_(std::move(sources))
{
}

auto HeartRateSensorDetector::process() const -> HeartRateResult
{
    if (sources_.size() == 1)
    {
        return sources_.front()->read();
    }

    for (const auto& source : sources_)
    {
        HeartRateResult const result = source->read();

        if (result.status == HeartRateStatus::Valid)
        {
            return result;
        }
    }

    return {
        .bpm = 0,
        .status = HeartRateStatus::NoData,
    };
}
