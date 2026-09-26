#ifndef EYETRACKING_IRISDETECTION_DETECTIONPOSTPROCESS_HPP
#define EYETRACKING_IRISDETECTION_DETECTIONPOSTPROCESS_HPP

#include "opencv2/core.hpp"
#include <algorithm>
#include <functional>
#include <string>
#include <vector>

constexpr int CLASS_ID = 0;
constexpr float MIN_THRESHOLD = 0.75f;
constexpr int DETECTION_SIZE = 128;
constexpr int NUM_BOXES = 896;
constexpr int NUM_COORD = 16;
constexpr int NUM_SIZES = 2;

namespace my {

    struct AnchorOptions {
        // 2 x 16 x 16 and 6 x 8 x 8 --> 896
        const int sizes[NUM_SIZES] = {16, 8};
        const int numLayers[NUM_SIZES] = {2, 6};

        // The offset for the center of anchors.
        const float offsetX = 0.5f;
        const float offsetY = 0.5f;
    };


    struct Detection {
        cv::Rect2f roi;
        float score;
        int classId;

        Detection() : score(), classId(-1) {}
        Detection(float score, int classId, cv::Rect2f roi) :
            score(score), classId(classId), roi(roi) {}
        // ~Detection() = default;
    };

    /*
    A helper class converts the output from Mediapipe Face Detection to Face box.
    */
    class DetectionPostProcess {
        public:
            DetectionPostProcess();
            // ~DetectionPostProcess() = default;
            [[nodiscard]] auto getHighestScoreDetection
            (const std::vector<float>& rawBoxes, const std::vector<float>& scores) const -> Detection;

        private:
            [[nodiscard]] auto decodeBox(const std::vector<float>& rawBoxes, int index) const -> cv::Rect2f;

            std::vector<cv::Rect2f> m_anchors;
    };
}

#endif // EYETRACKING_IRISDETECTION_DETECTIONPOSTPROCESS_HPP