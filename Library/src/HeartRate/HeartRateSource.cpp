#include "HeartRate/HeartRateSource.hpp"

#include "HeartRate/UFeelHeartRateStatus.hpp"

#include <mutex>

void LatestReading::set(
    HeartRateResult result, Clock::time_point now
)
{
    std::scoped_lock const lock(mutex_);
    result_ = result;
    time_ = now;
    hasData_ = true;
}

auto LatestReading::get(
    Clock::time_point now
) const -> HeartRateResult
{
    std::scoped_lock const lock(mutex_);

    if (!hasData_ || now - time_ > kStaleAfter)
    {
        return {.bpm = 0, .status = HeartRateStatus::NoData};
    }

    return result_;
}
