#ifndef HEARTRATE_HEARTRATEDETECTOR_HPP
#define HEARTRATE_HEARTRATEDETECTOR_HPP

#include "UFeelHeartRateStatus.hpp"

#include <cstdint>

struct HeartRateResult
{
    std::uint32_t bpm;
    HeartRateStatus status;
};

class HeartRateSensorDetector
{
public:

    HeartRateSensorDetector() = default;

    ~HeartRateSensorDetector() = default;

    HeartRateSensorDetector(const HeartRateSensorDetector&) = delete;
    auto operator=(const HeartRateSensorDetector&) -> HeartRateSensorDetector& = delete;
    HeartRateSensorDetector(HeartRateSensorDetector&&) = delete;
    auto operator=(HeartRateSensorDetector&&) -> HeartRateSensorDetector& = delete;

    [[nodiscard]]
    static auto process() -> HeartRateResult;

private:

    std::uint32_t bpm_{};
};

#endif // HEARTRATE_HEARTRATEDETECTOR_HPP
