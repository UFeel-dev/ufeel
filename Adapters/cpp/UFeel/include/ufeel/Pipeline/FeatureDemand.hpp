#ifndef UFEEL_PIPELINE_FEATUREDEMAND_HPP
#define UFEEL_PIPELINE_FEATUREDEMAND_HPP

#include "ufeel/Model/Detection.hpp"
#include "ufeel/Pipeline/Clock.hpp"

#include <array>
#include <chrono>
#include <mutex>
#include <optional>

namespace ufeel
{

class FeatureDemand
{
public:

    explicit FeatureDemand(Clock::duration timeout = std::chrono::seconds(2));

    void request(DetectionType features, Clock::time_point now = Clock::now());
    void pin(DetectionType features);
    void unpin(DetectionType features);

    [[nodiscard]] auto active(Clock::time_point now = Clock::now()) const -> DetectionType;

private:

    void addPins(DetectionType features, int delta);

    Clock::duration timeout_;
    mutable std::mutex mutex_;
    std::array<std::optional<Clock::time_point>, singleDetectionTypes.size()> lastRequest_;
    std::array<int, singleDetectionTypes.size()> pins_{};
};

} // namespace ufeel

#endif // UFEEL_PIPELINE_FEATUREDEMAND_HPP
