#include "Emotions/Inference/FaceDetector.hpp"

#include "opencv2/core/cvstd_wrapper.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"
#include "opencv2/dnn/dnn.hpp"
#include "opencv2/objdetect/face.hpp"

#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{

constexpr int kFirstEyeColumn = 4;
constexpr int kSecondEyeColumn = 6;

auto requireDetector(
    cv::Ptr<cv::FaceDetectorYN> detector, const std::string& error
) -> cv::Ptr<cv::FaceDetectorYN>
{
    if (detector.empty())
    {
        throw std::runtime_error(error);
    }

    return detector;
}

} // namespace

FaceDetector::FaceDetector(
    const std::string& modelPath, float scoreThreshold, float nmsThreshold, int topK
)
    : detector_(requireDetector(
          cv::FaceDetectorYN::create(
              modelPath,
              "",
              cv::Size(320, 320),
              scoreThreshold,
              nmsThreshold,
              topK,
              cv::dnn::DNN_BACKEND_OPENCV,
              cv::dnn::DNN_TARGET_CPU
          ),
          "Failed to load face detector: " + modelPath
      ))
{
}

FaceDetector::FaceDetector(
    cv::Ptr<cv::FaceDetectorYN> detector, DetectorTag
)
    : detector_(requireDetector(std::move(detector), "Face detector must not be null"))
{
}

auto FaceDetector::fromDetector(
    cv::Ptr<cv::FaceDetectorYN> detector
) -> FaceDetector
{
    return FaceDetector(std::move(detector), DetectorTag{});
}

auto FaceDetector::detect(
    const cv::Mat& image
) -> std::optional<FaceDetection>
{
    detector_->setInputSize(image.size());

    cv::Mat faces;
    detector_->detect(image, faces);

    if (faces.empty())
    {
        return std::nullopt;
    }

    int largest = 0;
    float largestArea = 0.0F;
    for (int row = 0; row < faces.rows; ++row)
    {
        const float area = faces.at<float>(row, 2) * faces.at<float>(row, 3);
        if (area > largestArea)
        {
            largestArea = area;
            largest = row;
        }
    }

    const auto* face = faces.ptr<float>(largest);

    cv::Point2f first(face[kFirstEyeColumn], face[kFirstEyeColumn + 1]);
    cv::Point2f second(face[kSecondEyeColumn], face[kSecondEyeColumn + 1]);
    if (first.x > second.x)
    {
        std::swap(first, second);
    }

    return FaceDetection{
        .box = cv::Rect2f(face[0], face[1], face[2], face[3]),
        .leftEye = first,
        .rightEye = second,
    };
}
