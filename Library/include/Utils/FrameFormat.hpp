#ifndef UTILS_FRAMEFORMAT_HPP
#define UTILS_FRAMEFORMAT_HPP

#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/imgproc.hpp"

inline auto toBgr(
    const cv::Mat& frame
) -> cv::Mat
{
    cv::Mat bgr;
    switch (frame.type())
    {
    case CV_8UC1:
        cv::cvtColor(frame, bgr, cv::COLOR_GRAY2BGR);
        return bgr;
    case CV_8UC4:
        cv::cvtColor(frame, bgr, cv::COLOR_BGRA2BGR);
        return bgr;
    default:
        return frame;
    }
}

#endif // UTILS_FRAMEFORMAT_HPP
