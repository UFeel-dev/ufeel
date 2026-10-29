#include "Emotions/Inference/FaceCrop.hpp"

#include "Emotions/Inference/FaceDetector.hpp"
#include "opencv2/core/cvdef.h"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"
#include "opencv2/imgproc.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace
{

auto resizeTo(
    const cv::Mat& image, const cv::Size& size
) -> cv::Mat
{
    const bool shrinking = image.cols > size.width || image.rows > size.height;

    cv::Mat resized;
    cv::resize(image, resized, size, 0.0, 0.0, shrinking ? cv::INTER_AREA : cv::INTER_LINEAR);
    return resized;
}

} // namespace

FaceCrop::FaceCrop(
    FaceDetector detector, int size, float margin
)
    : detector_(std::move(detector))
    , size_(size)
    , margin_(margin)
{
}

auto FaceCrop::operator()(
    const cv::Mat& image
) -> cv::Mat
{
    const auto detection = detector_.detect(image);
    if (!detection)
    {
        return resizeTo(image, cv::Size(size_, size_));
    }

    const auto& box = detection->box;
    const float marginX = margin_ * box.width;
    const float marginY = margin_ * box.height;

    const int x1 = std::max(static_cast<int>(box.x - marginX), 0);
    const int y1 = std::max(static_cast<int>(box.y - marginY), 0);
    const int x2 = std::min(static_cast<int>(box.x + box.width + marginX), image.cols);
    const int y2 = std::min(static_cast<int>(box.y + box.height + marginY), image.rows);

    if (x2 <= x1 || y2 <= y1)
    {
        return resizeTo(image, cv::Size(size_, size_));
    }

    const cv::Mat face = image(cv::Rect(x1, y1, x2 - x1, y2 - y1));

    const cv::Point2f eyeVector = detection->rightEye - detection->leftEye;
    const double angle = std::atan2(eyeVector.y, eyeVector.x) * 180.0 / CV_PI;
    const cv::Point2f center(
        static_cast<float>(face.cols - 1) / 2.0F, static_cast<float>(face.rows - 1) / 2.0F
    );

    cv::Mat aligned;
    cv::warpAffine(face, aligned, cv::getRotationMatrix2D(center, angle, 1.0), face.size());

    return resizeAndPad(aligned);
}

auto FaceCrop::resizeAndPad(
    const cv::Mat& image
) const -> cv::Mat
{
    const double scale = static_cast<double>(size_) / std::max(image.cols, image.rows);
    const cv::Size newSize(
        std::max(1, static_cast<int>(image.cols * scale)),
        std::max(1, static_cast<int>(image.rows * scale))
    );

    cv::Mat padded = cv::Mat::zeros(size_, size_, CV_8UC3);
    resizeTo(image, newSize)
        .copyTo(padded(
            cv::Rect(
                (size_ - newSize.width) / 2,
                (size_ - newSize.height) / 2,
                newSize.width,
                newSize.height
            )
        ));

    return padded;
}
