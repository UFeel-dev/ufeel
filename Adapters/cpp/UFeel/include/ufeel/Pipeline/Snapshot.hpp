#ifndef UFEEL_PIPELINE_SNAPSHOT_HPP
#define UFEEL_PIPELINE_SNAPSHOT_HPP

#include "ufeel/Model/EmotionResult.hpp"
#include "ufeel/Model/EyeTrackingResult.hpp"
#include "ufeel/Model/HeartRateResult.hpp"
#include "ufeel/Model/SpeechToTextResult.hpp"

#include <cstdint>
#include <string>

namespace ufeel
{

struct Snapshot
{
    std::uint64_t sequence = 0;
    bool frameAvailable = false;
    std::string message;
    EmotionResult emotions;
    EyeTrackingResult eyeTracking;
    SpeechToTextResult speechToText;
    HeartRateResult heartRate;
};

} // namespace ufeel

#endif // UFEEL_PIPELINE_SNAPSHOT_HPP
