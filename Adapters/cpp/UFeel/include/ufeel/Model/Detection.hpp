#ifndef UFEEL_MODEL_DETECTION_HPP
#define UFEEL_MODEL_DETECTION_HPP

#include "ufeel/Model/Flags.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace ufeel
{

enum class DetectionType : std::uint8_t
{
    None = 0U,
    Emotions = 1U << 0U,
    EyeTracking = 1U << 1U,
    SpeechToText = 1U << 2U,
    HeartRate = 1U << 3U,
};

template <> struct EnableFlags<DetectionType> : std::true_type
{
};

inline constexpr std::array<DetectionType, 4> singleDetectionTypes{
    DetectionType::Emotions,
    DetectionType::EyeTracking,
    DetectionType::SpeechToText,
    DetectionType::HeartRate,
};

inline constexpr DetectionType allDetectionTypes =
    DetectionType::Emotions | DetectionType::EyeTracking | DetectionType::SpeechToText |
    DetectionType::HeartRate;

inline constexpr DetectionType imageDetectionTypes =
    DetectionType::Emotions | DetectionType::EyeTracking;

[[nodiscard]] auto detectionIndex(DetectionType feature) -> std::size_t;

} // namespace ufeel

#endif // UFEEL_MODEL_DETECTION_HPP
