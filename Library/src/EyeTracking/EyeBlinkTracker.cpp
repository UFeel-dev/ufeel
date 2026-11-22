#include "EyeTracking/EyeBlinkTracker.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace
{

constexpr double OPEN_LEVEL_RISE_RATE = 0.3;
constexpr double OPEN_LEVEL_FALL_RATE = 0.01;
constexpr double INITIAL_OPEN_LEVEL = 0.2;

} // namespace

auto EyeBlinkTracker::Eye::update(
    double aspectRatio
) -> double
{
    if (openLevel <= 0.0)
    {
        openLevel = std::max(aspectRatio, INITIAL_OPEN_LEVEL);
    }

    double const openness = std::clamp(aspectRatio / openLevel, 0.0, 1.0);
    closed = closed ? openness < REOPEN_THRESHOLD : openness < CLOSE_THRESHOLD;

    if (!closed)
    {
        double const rate = aspectRatio > openLevel ? OPEN_LEVEL_RISE_RATE : OPEN_LEVEL_FALL_RATE;
        openLevel += rate * (aspectRatio - openLevel);
    }
    return openness;
}

auto EyeBlinkTracker::update(
    double leftAspectRatio, double rightAspectRatio, Clock::time_point now
) -> EyeBlinkState
{
    if (!std::isfinite(leftAspectRatio) || !std::isfinite(rightAspectRatio) ||
        leftAspectRatio <= 0.0 || rightAspectRatio <= 0.0)
    {
        interrupt();
        return state_;
    }

    state_.leftOpenness = static_cast<float>(left_.update(leftAspectRatio));
    state_.rightOpenness = static_cast<float>(right_.update(rightAspectRatio));
    state_.leftClosed = left_.closed;
    state_.rightClosed = right_.closed;

    bool const bothClosed = left_.closed && right_.closed;
    if (bothClosed && !closedSince_)
    {
        closedSince_ = now;
    }
    else if (!bothClosed && closedSince_)
    {
        if (now - *closedSince_ <= MAXIMUM_BLINK_DURATION)
        {
            ++state_.blinkCount;
        }
        closedSince_.reset();
    }

    state_.eyesClosedSeconds =
        closedSince_ ? std::chrono::duration<float>(now - *closedSince_).count() : 0.0F;
    return state_;
}

void EyeBlinkTracker::interrupt()
{
    closedSince_.reset();
    left_.closed = false;
    right_.closed = false;
    state_.leftClosed = false;
    state_.rightClosed = false;
    state_.leftOpenness = 0.0F;
    state_.rightOpenness = 0.0F;
    state_.eyesClosedSeconds = 0.0F;
}

auto EyeBlinkTracker::state() const -> const EyeBlinkState&
{
    return state_;
}
