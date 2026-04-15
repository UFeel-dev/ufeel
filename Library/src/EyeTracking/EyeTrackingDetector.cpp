/*
** EPITECH PROJECT, 2026
** test_lib
** File description:
** EyeTrackingDetector
*/

#include "EyeTracking/EyeTrackingDetector.hpp"
#include "opencv2/core.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <iostream>
#include <map>
#include <string>

EyeTrackingDetector::EyeTrackingDetector() : irisLandmarker_("./models")
{

}

EyeTrackingDetector::~EyeTrackingDetector()
{
}

void EyeTrackingDetector::toggleEyeTrackingDetection(bool state)
{
    processEnable_ = state;
    std::cout << "[EyeTrackingDetector] eye tracking detection " << (state ? "enabled" : "disabled") << std::endl;
}

std::map<std::string, bool> EyeTrackingDetector::process(const cv::Mat& frame)
{
    std::map<std::string, bool> result = {
        {"left",false},{"right",false},
        {"up",false},{"down",false},{"center",false}
    };

    cv::Mat rframe = frame.clone();

    cv::flip(rframe, rframe, 1);

    irisLandmarker_.loadImageToInput(rframe);
    irisLandmarker_.runInference();


    auto leftEye  = irisLandmarker_.getAllEyeLandmarks(true, false);
    auto leftIris = irisLandmarker_.getAllEyeLandmarks(true, true);

    auto rightEye  = irisLandmarker_.getAllEyeLandmarks(false, false);
    auto rightIris = irisLandmarker_.getAllEyeLandmarks(false, true);


    for (auto landmark: leftEye) {
        cv::circle(rframe, landmark, 2, cv::Scalar(255, 0, 255), -1);
    }
    for (auto landmark: leftIris) {
        cv::circle(rframe, landmark, 2, cv::Scalar(0, 0, 255), -1);
    }

    for (auto landmark: rightEye) {
        cv::circle(rframe, landmark, 2, cv::Scalar(255, 0, 255), -1);
    }
    for (auto landmark: rightIris) {
        cv::circle(rframe, landmark, 2, cv::Scalar(0, 0, 255), -1);
    }

    cv::imshow("Face detector", rframe);

    // auto compute = [](const auto& eye, const auto& iris)
    // {
    //     float minX = std::numeric_limits<float>::max();
    //     float maxX = std::numeric_limits<float>::lowest();
    //     float minY = std::numeric_limits<float>::max();
    //     float maxY = std::numeric_limits<float>::lowest();

    //     for (auto& p : eye) {
    //         float x = static_cast<float>(p.x);
    //         float y = static_cast<float>(p.y);

    //         minX = std::min(minX, x);
    //         maxX = std::max(maxX, x);
    //         minY = std::min(minY, y);
    //         maxY = std::max(maxY, y);
    //     }

    //     float irisX = 0.f;
    //     float irisY = 0.f;

    //     for (auto& p : iris) {
    //         irisX += static_cast<float>(p.x);
    //         irisY += static_cast<float>(p.y);
    //     }

    //     irisX /= static_cast<float>(iris.size());
    //     irisY /= static_cast<float>(iris.size());

    //     float nx = (irisX - minX) / (maxX - minX);
    //     float ny = (irisY - minY) / (maxY - minY);

    //     return std::pair<float,float>{nx, ny};
    // };

    // auto [lx, ly] = compute(leftEye, leftIris);
    // auto [rx, ry] = compute(rightEye, rightIris);

    // float x = (lx + rx) * 0.5f;
    // float y = (ly + ry) * 0.5f;

    // if (x < 0.35f)
    //     result["right"] = true;
    // else if (x > 0.65f)
    //     result["left"] = true;
    // else
    //     result["center"] = true;

    // if (y < 0.35f)
    //     result["up"] = true;
    // else if (y > 0.65f)
    //     result["down"] = true;

    return result;
}

void EyeTrackingDetector::close()
{
}