#ifndef HEARTRATE_HEARTRATEDETECTOR_HPP
#define HEARTRATE_HEARTRATEDETECTOR_HPP

class HeartRateSensorDetector
{
    public:

        explicit HeartRateSensorDetector();

        ~HeartRateSensorDetector();

        HeartRateSensorDetector(const HeartRateSensorDetector&) = delete;
        auto operator=(const HeartRateSensorDetector&) -> HeartRateSensorDetector& = delete;
        HeartRateSensorDetector(HeartRateSensorDetector&&) = delete;
        auto operator=(HeartRateSensorDetector&&) -> HeartRateSensorDetector& = delete;

        [[nodiscard]]
        auto process() const -> int;
};

#endif // HEARTRATE_HEARTRATEDETECTOR_HPP
