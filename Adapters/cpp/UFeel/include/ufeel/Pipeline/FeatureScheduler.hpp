#ifndef UFEEL_PIPELINE_FEATURESCHEDULER_HPP
#define UFEEL_PIPELINE_FEATURESCHEDULER_HPP

#include "ufeel/Model/Detection.hpp"
#include "ufeel/Pipeline/Clock.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <mutex>
#include <optional>

namespace ufeel
{

struct FeatureIntervals
{
    Clock::duration emotions = std::chrono::milliseconds(250);
    Clock::duration eyeTracking = std::chrono::milliseconds(50);
    Clock::duration speechToText = std::chrono::milliseconds(100);
    Clock::duration heartRate = std::chrono::seconds(1);
};

class FeatureScheduler
{
public:

    explicit FeatureScheduler(FeatureIntervals intervals = {});

    void setOverride(DetectionType feature, Clock::duration interval);
    void clearOverride(DetectionType feature);

    [[nodiscard]] auto takeDue(Clock::time_point now, DetectionType active) -> DetectionType;

private:

    [[nodiscard]] auto overdue(std::size_t index, Clock::time_point now) const -> double;

    std::mutex mutex_;
    std::array<Clock::duration, singleDetectionTypes.size()> intervals_;
    std::array<std::optional<Clock::duration>, singleDetectionTypes.size()> overrides_;
    std::array<std::optional<Clock::time_point>, singleDetectionTypes.size()> lastRun_;
};

} // namespace ufeel

#endif // UFEEL_PIPELINE_FEATURESCHEDULER_HPP
