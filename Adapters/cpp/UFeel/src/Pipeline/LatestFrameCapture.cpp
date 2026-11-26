#include "ufeel/Pipeline/LatestFrameCapture.hpp"

#include "ufeel/Model/Status.hpp"
#include "ufeel/Native/Processor.hpp"
#include "ufeel/Pipeline/Clock.hpp"

#include <algorithm>
#include <chrono>

namespace ufeel
{

namespace
{

constexpr int maxQueuedFrames = 4;
constexpr double framePeriodSmoothing = 0.1;
constexpr auto blockingReadThreshold = std::chrono::milliseconds(4);
constexpr auto minFramePeriod = std::chrono::milliseconds(5);
constexpr auto maxFramePeriod = std::chrono::milliseconds(250);

} // namespace

auto LatestFrameCapture::capture(
    Processor& processor
) -> Status
{
    constexpr int readLimit = maxQueuedFrames + 1;

    int reads = readLimit;
    if (lastCapture_)
    {
        const Clock::rep framesSinceCapture = (Clock::now() - *lastCapture_) / framePeriod_;
        reads = static_cast<int>(std::clamp<Clock::rep>(framesSinceCapture, 1, readLimit));
    }

    bool waited = false;
    int performed = 0;
    while (performed < reads)
    {
        const auto readStart = Clock::now();
        const Status status = processor.updateFrame();
        ++performed;

        if (status != Status::Ok)
        {
            reset();
            return status;
        }

        waited = Clock::now() - readStart >= blockingReadThreshold;
        if (waited)
        {
            break;
        }
    }

    const auto now = Clock::now();
    if (waited && performed == 1 && lastReadWaited_ && lastCapture_)
    {
        updateFramePeriod(now - *lastCapture_);
    }

    lastReadWaited_ = waited;
    lastCapture_ = now;
    return Status::Ok;
}

void LatestFrameCapture::reset()
{
    lastCapture_.reset();
    lastReadWaited_ = false;
}

void LatestFrameCapture::updateFramePeriod(
    Clock::duration sample
)
{
    if (sample < minFramePeriod || sample > maxFramePeriod)
    {
        return;
    }

    framePeriod_ +=
        std::chrono::duration_cast<Clock::duration>((sample - framePeriod_) * framePeriodSmoothing);
}

} // namespace ufeel
