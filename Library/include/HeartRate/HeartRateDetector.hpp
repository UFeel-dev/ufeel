#ifndef HEARTRATE_HEARTRATEDETECTOR_HPP
#define HEARTRATE_HEARTRATEDETECTOR_HPP

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
        static auto process() -> int;
};

#endif // HEARTRATE_HEARTRATEDETECTOR_HPP
