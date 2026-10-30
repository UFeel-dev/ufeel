#include "Emotions/Inference/FaceDetector.hpp"

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

} // namespace

FaceDetector::FaceDetector(
    const std::string& modelPath, float scoreThreshold, float nmsThreshold, int topK
)
    : detector_(
          cv::FaceDetectorYN::create(
              modelPath,
              "",
              cv::Size(320, 320),
              scoreThreshold,
              nmsThreshold,
              topK,
              cv::dnn::DNN_BACKEND_OPENCV,
              cv::dnn::DNN_TARGET_CPU
          )
      )
{
    if (detector_.empty())
    {
        throw std::runtime_error("Failed to load face detector: " + modelPath);
    }
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
