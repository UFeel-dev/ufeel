#ifndef EMOTIONS_EMOTIONLABELS_HPP
#define EMOTIONS_EMOTIONLABELS_HPP

#include <array>
#include <string_view>

inline constexpr std::array<std::string_view, 8> kEmotionLabels{
    "angry",
    "contemptuous",
    "disgusted",
    "fearful",
    "happy",
    "neutral",
    "sad",
    "surprised",
};

using EmotionScores = std::array<float, kEmotionLabels.size()>;

#endif // EMOTIONS_EMOTIONLABELS_HPP
