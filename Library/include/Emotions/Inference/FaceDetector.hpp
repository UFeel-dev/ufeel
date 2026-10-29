#ifndef EMOTIONS_INFERENCE_FACEDETECTOR_HPP
#define EMOTIONS_INFERENCE_FACEDETECTOR_HPP

#include "opencv2/core/cvstd_wrapper.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"
#include "opencv2/objdetect/face.hpp"

#include <optional>
#include <string>

struct FaceDetection
{
    cv::Rect2f box;
    cv::Point2f leftEye;
    cv::Point2f rightEye;
};

class FaceDetector
{
public:

    explicit FaceDetector(
        const std::string& modelPath,
        float scoreThreshold = 0.9F,
        float nmsThreshold = 0.3F,
        int topK = 5000
    );

    [[nodiscard]] auto detect(const cv::Mat& image) -> std::optional<FaceDetection>;

private:

    cv::Ptr<cv::FaceDetectorYN> detector_;
};

#endif // EMOTIONS_INFERENCE_FACEDETECTOR_HPP
