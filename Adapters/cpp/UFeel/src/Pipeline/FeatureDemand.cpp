#include "ufeel/Pipeline/FeatureDemand.hpp"

#include "ufeel/Model/Detection.hpp"
#include "ufeel/Model/Flags.hpp"
#include "ufeel/Pipeline/Clock.hpp"

#include <cstddef>
#include <mutex>

namespace ufeel
{

FeatureDemand::FeatureDemand(
    Clock::duration timeout
)
    : timeout_(timeout)
{
}

void FeatureDemand::request(
    DetectionType features, Clock::time_point now
)
{
    const std::scoped_lock lock(mutex_);
    for (std::size_t index = 0; index < singleDetectionTypes.size(); ++index)
    {
        if (hasAny(features, singleDetectionTypes.at(index)))
        {
            lastRequest_.at(index) = now;
        }
    }
}

void FeatureDemand::pin(
    DetectionType features
)
{
    addPins(features, 1);
}

void FeatureDemand::unpin(
    DetectionType features
)
{
    addPins(features, -1);
}

auto FeatureDemand::active(
    Clock::time_point now
) const -> DetectionType
{
    const std::scoped_lock lock(mutex_);
    DetectionType active = DetectionType::None;
    for (std::size_t index = 0; index < singleDetectionTypes.size(); ++index)
    {
        const auto& lastRequest = lastRequest_.at(index);
        const bool requested = lastRequest && now - *lastRequest <= timeout_;
        if (requested || pins_.at(index) > 0)
        {
            active |= singleDetectionTypes.at(index);
        }
    }
    return active;
}

void FeatureDemand::addPins(
    DetectionType features, int delta
)
{
    const std::scoped_lock lock(mutex_);
    for (std::size_t index = 0; index < singleDetectionTypes.size(); ++index)
    {
        if (hasAny(features, singleDetectionTypes.at(index)))
        {
            pins_.at(index) += delta;
        }
    }
}

} // namespace ufeel
