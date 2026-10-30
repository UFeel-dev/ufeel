#include "Emotions/Inference/HeadCalibration.hpp"

#include "Emotions/EmotionLabels.hpp"

#include <algorithm>
#include <cstddef>
#include <map>
#include <stdexcept>
#include <string>

namespace
{

constexpr float kDefaultThreshold = 0.5F;

} // namespace

HeadCalibration::HeadCalibration()
{
    thresholds_.fill(kDefaultThreshold);
}

HeadCalibration::HeadCalibration(
    const std::map<std::string, float>& thresholds
)
    : HeadCalibration()
{
    for (const auto& [label, threshold] : thresholds)
    {
        const auto* const it = std::ranges::find(kEmotionLabels, label);
        if (it == kEmotionLabels.end())
        {
            throw std::invalid_argument("Unknown emotion in thresholds: " + label);
        }
        thresholds_.at(static_cast<std::size_t>(it - kEmotionLabels.begin())) = threshold;
    }
}

auto HeadCalibration::select(
    const EmotionScores& scores
) const -> std::map<std::string, float>
{
    std::map<std::string, float> selected;

    for (std::size_t i = 0; i < scores.size(); ++i)
    {
        if (scores.at(i) > thresholds_.at(i))
        {
            selected.emplace(std::string(kEmotionLabels.at(i)), scores.at(i));
        }
    }

    return selected;
}
