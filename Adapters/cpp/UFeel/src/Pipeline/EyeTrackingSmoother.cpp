#include "ufeel/Pipeline/EyeTrackingSmoother.hpp"

#include "ufeel/Model/EyeTrackingResult.hpp"
#include "ufeel/Pipeline/Clock.hpp"

#include <chrono>
#include <cmath>

namespace ufeel
{

namespace
{

constexpr double timeConstantSeconds = 0.08;
constexpr auto resetGap = std::chrono::milliseconds(500);

} // namespace

void EyeTrackingSmoother::reset()
{
    lastTimestamp_.reset();
}

auto EyeTrackingSmoother::smooth(
    const EyeTrackingResult& result, Clock::time_point now
) -> EyeTrackingResult
{
    if (!result.isValid())
    {
        reset();
        return result;
    }

    const auto elapsed = lastTimestamp_ ? now - *lastTimestamp_ : Clock::duration::zero();
    const bool restart =
        !lastTimestamp_ || elapsed <= Clock::duration::zero() || elapsed > resetGap;
    lastTimestamp_ = now;

    if (restart)
    {
        smoothedX_ = result.estimatedX;
        smoothedY_ = result.estimatedY;
        return result;
    }

    const double alpha =
        1.0 - std::exp(-std::chrono::duration<double>(elapsed).count() / timeConstantSeconds);
    smoothedX_ += alpha * (result.estimatedX - smoothedX_);
    smoothedY_ += alpha * (result.estimatedY - smoothedY_);

    EyeTrackingResult smoothed = result;
    smoothed.estimatedX = static_cast<float>(smoothedX_);
    smoothed.estimatedY = static_cast<float>(smoothedY_);
    return smoothed;
}

} // namespace ufeel
