#ifndef UFEEL_MODEL_EMOTIONRESULT_HPP
#define UFEEL_MODEL_EMOTIONRESULT_HPP

#include "ufeel.h"

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace ufeel
{

enum class EmotionStatus : std::uint8_t
{
    FaceUnavailable = UFEEL_EMOTIONS_STATUS_FACE_UNAVAILABLE,
    Valid = UFEEL_EMOTIONS_STATUS_VALID,
    FrameUnavailable = UFEEL_EMOTIONS_STATUS_FRAME_UNAVAILABLE,
    InvalidProcessor = UFEEL_EMOTIONS_STATUS_INVALID_PROCESSOR,
    InternalError = UFEEL_EMOTIONS_STATUS_INTERNAL_ERROR,
    UnsupportedFeature = UFEEL_EMOTIONS_STATUS_UNSUPPORTED_FEATURE,
};

enum class Emotion : std::uint8_t
{
    Angry,
    Contemptuous,
    Disgusted,
    Fearful,
    Happy,
    Neutral,
    Sad,
    Surprised,
};

struct EmotionResult
{
    EmotionStatus status = EmotionStatus::FrameUnavailable;
    std::map<std::string, float, std::less<>> scores;

    [[nodiscard]] auto isValid() const -> bool
    {
        return status == EmotionStatus::Valid;
    }

    [[nodiscard]] auto dominant() const -> std::optional<Emotion>;
};

[[nodiscard]] auto parseEmotion(std::string_view label) -> std::optional<Emotion>;

[[nodiscard]] auto toString(Emotion emotion) -> std::string_view;

} // namespace ufeel

#endif // UFEEL_MODEL_EMOTIONRESULT_HPP
