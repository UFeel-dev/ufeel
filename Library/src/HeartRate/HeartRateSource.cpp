#include "HeartRate/HeartRateSource.hpp"

#include "HeartRate/UFeelHeartRateStatus.hpp"

#include <mutex>

void LatestReading::set(
    HeartRateResult result
)
{
    std::scoped_lock const lock(mutex_);
    result_ = result;
    time_ = Clock::now();
    hasData_ = true;
}

auto LatestReading::get() const -> HeartRateResult
{
    std::scoped_lock const lock(mutex_);

    if (!hasData_ || Clock::now() - time_ > kStaleAfter)
    {
        return {.bpm = 0, .status = HeartRateStatus::NoData};
    }

    return result_;
}
