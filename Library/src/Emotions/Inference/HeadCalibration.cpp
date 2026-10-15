#include "Emotions/Inference/HeadCalibration.hpp"

HeadCalibration::HeadCalibration(
    Probabilities thresholds
)
    : thresholds_(std::move(thresholds))
{
}

auto HeadCalibration::apply(
    const Probabilities& probs
) const -> HeadCalibration::Decisions
{
    Decisions result;

    for (const auto& [name, probability] : probs)
    {
        auto it = thresholds_.find(name);

        float threshold = (it != thresholds_.end()) ? it->second : 0.5f;

        result[name] = probability > threshold;
    }
    return result;
}

auto HeadCalibration::applySoft(
    const Probabilities& probs
) const -> HeadCalibration::Probabilities
{
    Probabilities result;

    for (const auto& [name, probability] : probs)
    {
        auto it = thresholds_.find(name);

        float threshold = (it != thresholds_.end()) ? it->second : 0.5f;

        result[name] = probability - threshold;
    }

    return result;
}
