#ifndef EYETRACKING_GAZEKALMANFILTER_HPP
#define EYETRACKING_GAZEKALMANFILTER_HPP

#include "opencv2/core/types.hpp"

#include <chrono>

class GazeKalmanFilter
{

private:

    using Clock = std::chrono::steady_clock;

    struct Axis
    {
        double position = 0.0;
        double velocity = 0.0;
        double p00 = 1.0;
        double p01 = 0.0;
        double p10 = 0.0;
        double p11 = 1.0;
    };

    Axis x_;
    Axis y_;
    Clock::time_point lastTime_;
    bool initialized_ = false;
    unsigned rejectedCount_ = 0;

    static constexpr double PROCESS_NOISE = 0.5;
    static constexpr double OUTLIER_THRESHOLD = 64.0;
    static constexpr unsigned MAX_REJECTED_MEASUREMENTS = 2;

    static void resetAxis(Axis& axis, double position);
    static void predict(Axis& axis, double dt);
    static auto correct(Axis& axis, double measurement, double noise) -> bool;

    [[nodiscard]] auto currentPosition() const -> cv::Point2f;
    void initialize(cv::Point2f measurement, Clock::time_point now);

public:

    void reset();

    auto update(cv::Point2f measurement, double quality, Clock::time_point now = Clock::now())
        -> cv::Point2f;
};

#endif // EYETRACKING_GAZEKALMANFILTER_HPP
