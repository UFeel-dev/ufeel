#ifndef GAZEKALMANFILTER_HPP
#define GAZEKALMANFILTER_HPP

#include "opencv2/core/types.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>

class GazeKalmanFilter
{
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
    static constexpr double OUTLIER_THRESHOLD = 16.0;

    static void resetAxis(
        Axis& axis, double position
    )
    {
        axis.position = position;
        axis.velocity = 0.0;
        axis.p00 = 0.01;
        axis.p01 = 0.0;
        axis.p10 = 0.0;
        axis.p11 = 1.0;
    }

    static void predict(
        Axis& axis, double dt
    )
    {
        double const oldP00 = axis.p00;
        double const oldP01 = axis.p01;
        double const oldP10 = axis.p10;
        double const oldP11 = axis.p11;

        axis.position += dt * axis.velocity;

        double const dt2 = dt * dt;
        double const dt3 = dt2 * dt;
        double const dt4 = dt2 * dt2;

        axis.p00 = oldP00 + (dt * (oldP01 + oldP10)) + (dt2 * oldP11) + (PROCESS_NOISE * dt4 / 4.0);
        axis.p01 = oldP01 + (dt * oldP11) + (PROCESS_NOISE * dt3 / 2.0);
        axis.p10 = oldP10 + (dt * oldP11) + (PROCESS_NOISE * dt3 / 2.0);
        axis.p11 = oldP11 + (PROCESS_NOISE * dt2);
    }

    static auto correct(
        Axis& axis, double measurement, double noise
    ) -> bool
    {
        double const innovation = measurement - axis.position;
        double const innovationVariance = axis.p00 + noise;

        if (innovation * innovation / innovationVariance > OUTLIER_THRESHOLD)
        {
            return false;
        }

        double const k0 = axis.p00 / innovationVariance;
        double const k1 = axis.p10 / innovationVariance;

        double const oldP00 = axis.p00;
        double const oldP01 = axis.p01;

        axis.position += k0 * innovation;
        axis.velocity += k1 * innovation;

        axis.p00 = (1.0 - k0) * oldP00;
        axis.p01 = (1.0 - k0) * oldP01;
        axis.p10 -= k1 * oldP00;
        axis.p11 -= k1 * oldP01;

        return true;
    }

        public:

    void reset()
    {
        x_ = {};
        y_ = {};
        lastTime_ = {};
        initialized_ = false;
        rejectedCount_ = 0;
    }

    auto update(
        cv::Point2f measurement, double quality, Clock::time_point now = Clock::now()
    ) -> cv::Point2f
    {
        if (!initialized_)
        {
            resetAxis(x_, measurement.x);
            resetAxis(y_, measurement.y);
            lastTime_ = now;
            initialized_ = true;
            rejectedCount_ = 0;
            return measurement;
        }

        double dt = std::chrono::duration<double>(now - lastTime_).count();
        lastTime_ = now;

        if (!std::isfinite(dt) || dt <= 0.0 || dt > 0.5)
        {
            resetAxis(x_, measurement.x);
            resetAxis(y_, measurement.y);
            rejectedCount_ = 0;
            return measurement;
        }

        dt = std::clamp(dt, 0.001, 0.1);

        predict(x_, dt);
        predict(y_, dt);

        double const q = std::clamp(quality, 0.1, 1.0);
        double const noise = 0.0025 / (q * q);

        bool const acceptedX = correct(x_, measurement.x, noise);
        bool const acceptedY = correct(y_, measurement.y, noise);

        if (!acceptedX || !acceptedY)
        {
            ++rejectedCount_;

            if (rejectedCount_ >= 3)
            {
                resetAxis(x_, measurement.x);
                resetAxis(y_, measurement.y);
                rejectedCount_ = 0;
            }
        }
        else
        {
            rejectedCount_ = 0;
        }

        return {
            static_cast<float>(std::clamp(x_.position, 0.0, 1.0)),
            static_cast<float>(std::clamp(y_.position, 0.0, 1.0))};
    }
};

#endif
