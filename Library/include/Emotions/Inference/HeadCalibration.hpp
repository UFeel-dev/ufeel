#ifndef EMOTIONS_INFERENCE_HEADCALIBRATION_HPP
#define EMOTIONS_INFERENCE_HEADCALIBRATION_HPP

#include "Emotions/EmotionLabels.hpp"

#include <map>
#include <string>

class HeadCalibration
{
public:

    HeadCalibration();

    explicit HeadCalibration(const std::map<std::string, float>& thresholds);

    [[nodiscard]] auto select(const EmotionScores& scores) const -> std::map<std::string, float>;

private:

    EmotionScores thresholds_{};
};

#endif // EMOTIONS_INFERENCE_HEADCALIBRATION_HPP
