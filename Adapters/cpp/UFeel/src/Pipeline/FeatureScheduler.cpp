#include "ufeel/Pipeline/FeatureScheduler.hpp"

#include "ufeel/Model/Detection.hpp"
#include "ufeel/Model/Flags.hpp"
#include "ufeel/Pipeline/Clock.hpp"

#include <chrono>
#include <cstddef>
#include <limits>
#include <mutex>
#include <optional>

namespace ufeel
{

FeatureScheduler::FeatureScheduler(
    FeatureIntervals intervals
)
    : intervals_{
          intervals.emotions,
          intervals.eyeTracking,
          intervals.speechToText,
          intervals.heartRate,
      }
{
}

void FeatureScheduler::setOverride(
    DetectionType feature, Clock::duration interval
)
{
    const std::size_t index = detectionIndex(feature);
    const std::scoped_lock lock(mutex_);
    overrides_.at(index) = interval;
}

void FeatureScheduler::clearOverride(
    DetectionType feature
)
{
    const std::size_t index = detectionIndex(feature);
    const std::scoped_lock lock(mutex_);
    overrides_.at(index).reset();
}

auto FeatureScheduler::takeDue(
    Clock::time_point now, DetectionType active
) -> DetectionType
{
    const std::scoped_lock lock(mutex_);
    DetectionType due = DetectionType::None;
    std::optional<std::size_t> imageIndex;
    double imageOverdue = 0.0;

    for (std::size_t index = 0; index < singleDetectionTypes.size(); ++index)
    {
        const DetectionType feature = singleDetectionTypes.at(index);
        if (!hasAny(active, feature))
        {
            continue;
        }

        const double ratio = overdue(index, now);
        if (ratio < 1.0)
        {
            continue;
        }

        if (!hasAny(imageDetectionTypes, feature))
        {
            lastRun_.at(index) = now;
            due |= feature;
        }
        else if (ratio > imageOverdue)
        {
            imageIndex = index;
            imageOverdue = ratio;
        }
    }

    if (imageIndex)
    {
        lastRun_.at(*imageIndex) = now;
        due |= singleDetectionTypes.at(*imageIndex);
    }

    return due;
}

auto FeatureScheduler::overdue(
    std::size_t index, Clock::time_point now
) const -> double
{
    const Clock::duration interval = overrides_.at(index).value_or(intervals_.at(index));
    const auto& lastRun = lastRun_.at(index);
    if (!lastRun || interval <= Clock::duration::zero())
    {
        return std::numeric_limits<double>::infinity();
    }

    return std::chrono::duration<double>(now - *lastRun) / std::chrono::duration<double>(interval);
}

} // namespace ufeel
