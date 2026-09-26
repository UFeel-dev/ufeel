/*
** EPITECH PROJECT, 2026
** test_lib
** File description:
** EyeTrackingDetector
*/

#ifndef EYETRACKING_EYETRACKINGDETECTOR_HPP
    #define EYETRACKING_EYETRACKINGDETECTOR_HPP

#include <opencv2/opencv.hpp>

#include "EyeTracking/IrisDetection/IrisLandmark.hpp"
#include <algorithm>

struct EyeDirections
{
    bool left;
    bool right;
    bool up;
    bool down;
    bool center;
};

class EyeTrackingDetector
{
    public:
        EyeTrackingDetector();
        ~EyeTrackingDetector();

        auto process(const cv::Mat& frame) -> std::map<std::string, bool>;
        void toggleEyeTrackingDetection(bool state);
        void close();

    private:

        bool processEnable_ = true;


        const std::vector<int> leftEyeIdx = {33,133,160,159,158,157,173};
        const std::vector<int> rightEyeIdx = {362,263,387,386,385,384,398};

        my::IrisLandmark irisLandmarker_;

};

#endif // EYETRACKING_EYETRACKINGDETECTOR_HPP
