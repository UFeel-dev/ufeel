#ifndef UFEEL_PIPELINE_LATESTFRAMECAPTURE_HPP
#define UFEEL_PIPELINE_LATESTFRAMECAPTURE_HPP

#include "ufeel/Model/Status.hpp"
#include "ufeel/Native/Processor.hpp"
#include "ufeel/Pipeline/Clock.hpp"

#include <chrono>
#include <optional>

namespace ufeel
{

class LatestFrameCapture
{
public:

    [[nodiscard]] auto capture(Processor& processor) -> Status;
    void reset();

private:

    void updateFramePeriod(Clock::duration sample);

    Clock::duration framePeriod_ =
        std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(1.0 / 30.0));
    std::optional<Clock::time_point> lastCapture_;
    bool lastReadWaited_ = false;
};

} // namespace ufeel

#endif // UFEEL_PIPELINE_LATESTFRAMECAPTURE_HPP
