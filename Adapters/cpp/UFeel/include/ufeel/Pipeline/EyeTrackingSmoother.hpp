#ifndef UFEEL_PIPELINE_EYETRACKINGSMOOTHER_HPP
#define UFEEL_PIPELINE_EYETRACKINGSMOOTHER_HPP

#include "ufeel/Model/EyeTrackingResult.hpp"
#include "ufeel/Pipeline/Clock.hpp"

#include <optional>

namespace ufeel
{

class EyeTrackingSmoother
{
public:

    void reset();

    [[nodiscard]] auto smooth(const EyeTrackingResult& result, Clock::time_point now = Clock::now())
        -> EyeTrackingResult;

private:

    double smoothedX_ = 0.0;
    double smoothedY_ = 0.0;
    std::optional<Clock::time_point> lastTimestamp_;
};

} // namespace ufeel

#endif // UFEEL_PIPELINE_EYETRACKINGSMOOTHER_HPP
