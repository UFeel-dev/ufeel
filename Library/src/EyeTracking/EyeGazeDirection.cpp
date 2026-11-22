#include "EyeTracking/EyeGazeDirection.hpp"

#include "opencv2/core/types.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace
{

constexpr std::size_t EYE_X = 0;
constexpr std::size_t EYE_Y = 1;
constexpr std::size_t HEAD_X = 2;
constexpr std::size_t HEAD_Y = 3;

constexpr double SMOOTHING_TIME_CONSTANT = 0.25;
constexpr double MAXIMUM_FRAME_GAP = 0.5;
constexpr std::size_t CALIBRATION_NEUTRAL_WEIGHT = EyeGazeDirection::NEUTRAL_WINDOW / 2;
constexpr std::size_t MINIMUM_CALIBRATION_SAMPLES = 5;
constexpr double MINIMUM_RANGE_RATIO = 0.25;
constexpr double MAXIMUM_RANGE_RATIO = 4.0;

[[nodiscard]] auto median(
    std::vector<double> values
) -> double
{
    auto const middle = values.begin() + static_cast<std::ptrdiff_t>(values.size() / 2);
    std::ranges::nth_element(values, middle);
    return *middle;
}

[[nodiscard]] auto nextDirection(
    int current, double value
) -> int
{
    if (current != 0 && value * current > EyeGazeDirection::DIRECTION_EXIT)
    {
        return current;
    }
    if (value > EyeGazeDirection::DIRECTION_ENTER)
    {
        return 1;
    }
    if (value < -EyeGazeDirection::DIRECTION_ENTER)
    {
        return -1;
    }
    return 0;
}

[[nodiscard]] auto fitAxis(
    const std::vector<std::pair<double, double>>& points, double defaultRange
) -> std::optional<std::pair<double, double>>
{
    double meanTarget = 0.0;
    double meanOffset = 0.0;
    for (auto const& [target, offset] : points)
    {
        meanTarget += target;
        meanOffset += offset;
    }
    meanTarget /= static_cast<double>(points.size());
    meanOffset /= static_cast<double>(points.size());

    double covariance = 0.0;
    double variance = 0.0;
    for (auto const& [target, offset] : points)
    {
        covariance += (target - meanTarget) * (offset - meanOffset);
        variance += (target - meanTarget) * (target - meanTarget);
    }
    if (variance < 1.0e-6)
    {
        return std::nullopt;
    }

    double const slope = covariance / variance;
    double const range = slope * 0.5;
    if (!std::isfinite(range) || range < defaultRange * MINIMUM_RANGE_RATIO ||
        range > defaultRange * MAXIMUM_RANGE_RATIO)
    {
        return std::nullopt;
    }
    return std::pair{meanOffset + (slope * (0.5 - meanTarget)), range};
}

} // namespace

auto EyeGazeDirection::update(
    const EyeGazeSample& sample, Clock::time_point now
) -> EyeGazeDirectionOutput
{
    addNeutralSample({sample.eyeX, sample.eyeY, sample.headX, sample.headY});

    double const rawX = ((sample.eyeX - neutral(EYE_X)) / eyeRangeX_) +
                        (headWeight_ * (sample.headX - neutral(HEAD_X)) / HEAD_RANGE_X);
    double const rawY = ((sample.eyeY - neutral(EYE_Y)) / eyeRangeY_) +
                        (headWeight_ * (sample.headY - neutral(HEAD_Y)) / HEAD_RANGE_Y);

    double const dt =
        lastTime_ ? std::chrono::duration<double>(now - *lastTime_).count() : MAXIMUM_FRAME_GAP;
    lastTime_ = now;
    if (dt <= 0.0 || dt >= MAXIMUM_FRAME_GAP)
    {
        smoothedX_ = rawX;
        smoothedY_ = rawY;
    }
    else
    {
        double const alpha = 1.0 - std::exp(-dt / SMOOTHING_TIME_CONSTANT);
        smoothedX_ += alpha * (rawX - smoothedX_);
        smoothedY_ += alpha * (rawY - smoothedY_);
    }

    horizontal_ = nextDirection(horizontal_, smoothedX_);
    vertical_ = nextDirection(vertical_, smoothedY_);

    return {
        .x = static_cast<float>(std::clamp(smoothedX_, -1.0, 1.0)),
        .y = static_cast<float>(std::clamp(smoothedY_, -1.0, 1.0)),
        .horizontal = horizontal_,
        .vertical = vertical_,
    };
}

void EyeGazeDirection::resetMotion()
{
    lastTime_.reset();
    smoothedX_ = 0.0;
    smoothedY_ = 0.0;
    horizontal_ = 0;
    vertical_ = 0;
}

void EyeGazeDirection::setHeadWeight(
    double weight
)
{
    headWeight_ = std::clamp(weight, 0.0, 1.0);
}

auto EyeGazeDirection::headWeight() const -> double
{
    return headWeight_;
}

void EyeGazeDirection::applyRange(
    const std::optional<EyeGazeRange>& range
)
{
    eyeRangeX_ = range ? range->rangeX : DEFAULT_EYE_RANGE_X;
    eyeRangeY_ = range ? range->rangeY : DEFAULT_EYE_RANGE_Y;
    if (!range)
    {
        return;
    }

    for (auto& history : neutralHistory_)
    {
        history.clear();
    }
    for (std::size_t sample = 0; sample < CALIBRATION_NEUTRAL_WEIGHT; ++sample)
    {
        neutralHistory_.at(EYE_X).push_back(range->neutralX);
        neutralHistory_.at(EYE_Y).push_back(range->neutralY);
    }
}

auto EyeGazeDirection::fitRange(
    const std::vector<std::pair<cv::Point2f, std::vector<EyeGazeSample>>>& targets
) -> std::optional<EyeGazeRange>
{
    std::vector<std::pair<double, double>> horizontal;
    std::vector<std::pair<double, double>> vertical;
    for (auto const& [target, samples] : targets)
    {
        if (samples.size() < MINIMUM_CALIBRATION_SAMPLES)
        {
            continue;
        }
        std::vector<double> eyeX;
        std::vector<double> eyeY;
        for (auto const& sample : samples)
        {
            eyeX.push_back(sample.eyeX);
            eyeY.push_back(sample.eyeY);
        }
        horizontal.emplace_back(target.x, median(eyeX));
        vertical.emplace_back(target.y, median(eyeY));
    }
    if (horizontal.size() < 2)
    {
        return std::nullopt;
    }

    auto const fittedX = fitAxis(horizontal, DEFAULT_EYE_RANGE_X);
    auto const fittedY = fitAxis(vertical, DEFAULT_EYE_RANGE_Y);
    if (!fittedX || !fittedY)
    {
        return std::nullopt;
    }
    return EyeGazeRange{
        .neutralX = fittedX->first,
        .neutralY = fittedY->first,
        .rangeX = fittedX->second,
        .rangeY = fittedY->second,
    };
}

void EyeGazeDirection::addNeutralSample(
    const std::array<double, CHANNEL_COUNT>& values
)
{
    for (std::size_t channel = 0; channel < CHANNEL_COUNT; ++channel)
    {
        auto& history = neutralHistory_.at(channel);
        history.push_back(values.at(channel));
        if (history.size() > NEUTRAL_WINDOW)
        {
            history.pop_front();
        }
    }
}

auto EyeGazeDirection::neutral(
    std::size_t channel
) const -> double
{
    auto const& history = neutralHistory_.at(channel);
    return history.empty() ? 0.0 : median({history.begin(), history.end()});
}
