#ifndef HEARTRATE_HEARTRATEDETECTOR_HPP
#define HEARTRATE_HEARTRATEDETECTOR_HPP

<<<<<<< HEAD
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
=======
class HeartRateSensorDetector
{
    public:

        explicit HeartRateSensorDetector();

        ~HeartRateSensorDetector();

        HeartRateSensorDetector(const HeartRateSensorDetector&) = delete;
        auto operator=(const HeartRateSensorDetector&) -> HeartRateSensorDetector& = delete;
        HeartRateSensorDetector(HeartRateSensorDetector&&) = delete;
        auto operator=(HeartRateSensorDetector&&) -> HeartRateSensorDetector& = delete;

        //void toggle(bool state);

        [[nodiscard]]
        auto process() const -> int;
>>>>>>> e88a471 (feat: add heartbeat in cpp library)
};

#endif // HEARTRATE_HEARTRATEDETECTOR_HPP
