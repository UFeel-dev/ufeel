#ifndef EYETRACKING_EYEGAZEDIRECTION_HPP
#define EYETRACKING_EYEGAZEDIRECTION_HPP

#include "opencv2/core/types.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <deque>
#include <optional>
#include <utility>
#include <vector>

struct EyeGazeSample
{
    double eyeX = 0.0;
    double eyeY = 0.0;
    double headX = 0.0;
    double headY = 0.0;
};

struct EyeGazeRange
{
    double neutralX = 0.0;
    double neutralY = 0.0;
    double rangeX = 0.0;
    double rangeY = 0.0;
};

struct EyeGazeDirectionOutput
{
    float x = 0.0F;
    float y = 0.0F;
    int horizontal = 0;
    int vertical = 0;
};

class EyeGazeDirection
{
public:

    using Clock = std::chrono::steady_clock;

    static constexpr double DEFAULT_EYE_RANGE_X = 0.02;
    static constexpr double DEFAULT_EYE_RANGE_Y = 0.016;
    static constexpr double HEAD_RANGE_X = 0.26;
    static constexpr double HEAD_RANGE_Y = 0.18;
    static constexpr double DIRECTION_ENTER = 0.6;
    static constexpr double DIRECTION_EXIT = 0.3;
    static constexpr std::size_t NEUTRAL_WINDOW = 600;

    auto update(const EyeGazeSample& sample, Clock::time_point now) -> EyeGazeDirectionOutput;

    void resetMotion();

    void setHeadWeight(double weight);
    [[nodiscard]] auto headWeight() const -> double;

    void applyRange(const std::optional<EyeGazeRange>& range);

    [[nodiscard]] static auto
    fitRange(const std::vector<std::pair<cv::Point2f, std::vector<EyeGazeSample>>>& targets)
        -> std::optional<EyeGazeRange>;

private:

    static constexpr std::size_t CHANNEL_COUNT = 4;

    void addNeutralSample(const std::array<double, CHANNEL_COUNT>& values);
    [[nodiscard]] auto neutral(std::size_t channel) const -> double;

    std::array<std::deque<double>, CHANNEL_COUNT> neutralHistory_;
    double headWeight_ = 0.0;
    double eyeRangeX_ = DEFAULT_EYE_RANGE_X;
    double eyeRangeY_ = DEFAULT_EYE_RANGE_Y;
    std::optional<Clock::time_point> lastTime_;
    double smoothedX_ = 0.0;
    double smoothedY_ = 0.0;
    int horizontal_ = 0;
    int vertical_ = 0;
};

#endif // EYETRACKING_EYEGAZEDIRECTION_HPP
