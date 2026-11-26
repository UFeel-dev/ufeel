#include "ufeel/Model/EmotionResult.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <string_view>
#include <utility>

namespace ufeel
{

namespace
{

constexpr std::array<std::pair<Emotion, std::string_view>, 8> emotionNames{
    {
        {Emotion::Angry, "Angry"},
        {Emotion::Contemptuous, "Contemptuous"},
        {Emotion::Disgusted, "Disgusted"},
        {Emotion::Fearful, "Fearful"},
        {Emotion::Happy, "Happy"},
        {Emotion::Neutral, "Neutral"},
        {Emotion::Sad, "Sad"},
        {Emotion::Surprised, "Surprised"},
    },
};

constexpr auto toLowerAscii(
    char character
) -> char
{
    return character >= 'A' && character <= 'Z' ? static_cast<char>(character - 'A' + 'a')
                                                : character;
}

auto equalsIgnoringCase(
    std::string_view left, std::string_view right
) -> bool
{
    return std::ranges::equal(left, right, [](char first, char second) -> bool {
        return toLowerAscii(first) == toLowerAscii(second);
    });
}

} // namespace

auto EmotionResult::dominant() const -> std::optional<Emotion>
{
    std::optional<Emotion> strongest;
    float best = -std::numeric_limits<float>::infinity();

    for (const auto& [label, score] : scores)
    {
        const auto emotion = parseEmotion(label);
        if (!emotion || std::isnan(score) || score <= best)
        {
            continue;
        }

        strongest = emotion;
        best = score;
    }

    return strongest;
}

auto parseEmotion(
    std::string_view label
) -> std::optional<Emotion>
{
    for (const auto& [emotion, name] : emotionNames)
    {
        if (equalsIgnoringCase(name, label))
        {
            return emotion;
        }
    }

    return std::nullopt;
}

auto toString(
    Emotion emotion
) -> std::string_view
{
    for (const auto& [candidate, name] : emotionNames)
    {
        if (candidate == emotion)
        {
            return name;
        }
    }

    return "Unknown";
}

} // namespace ufeel
