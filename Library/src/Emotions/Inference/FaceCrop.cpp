#include "Emotions/Inference/FaceCrop.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

MTCNNCropTransform::MTCNNCropTransform(
    int image_detection_size, std::shared_ptr<IFaceDetector> detector, float margin
)
    : imageDetectionSize_(image_detection_size)
    , margin_(margin)
    , detector_(std::move(detector))
{
    if (!detector_)
    {
        throw std::invalid_argument("MTCNN detector cannot be null");
    }
}

auto MTCNNCropTransform::alignAndCrop(
    const cv::Mat& image, const cv::Rect2f& box, const std::array<cv::Point2f, 5>& landmarks
) const -> cv::Mat
{
    float x1 = box.x;
    float y1 = box.y;

    float x2 = box.x + box.width;
    float y2 = box.y + box.height;

    const float w_box = x2 - x1;
    const float h_box = y2 - y1;

    x1 -= margin_ * w_box;
    y1 -= margin_ * h_box;

    x2 += margin_ * w_box;
    y2 += margin_ * h_box;

    x1 = std::max(x1, 0.0f);
    y1 = std::max(y1, 0.0f);

    x2 = std::min(x2, static_cast<float>(image.cols));

    y2 = std::min(y2, static_cast<float>(image.rows));

    const int ix1 = static_cast<int>(x1);
    const int iy1 = static_cast<int>(y1);
    const int ix2 = static_cast<int>(x2);
    const int iy2 = static_cast<int>(y2);

    if (ix2 <= ix1 || iy2 <= iy1)
    {
        throw std::runtime_error("Invalid face crop");
    }

    cv::Rect crop_rect(ix1, iy1, ix2 - ix1, iy2 - iy1);

    cv::Mat face_crop = image(crop_rect).clone();

    const cv::Point2f& left_eye = landmarks[0];
    const cv::Point2f& right_eye = landmarks[1];

    const float dx = right_eye.x - left_eye.x;
    const float dy = right_eye.y - left_eye.y;

    const double angle = std::atan2(dy, dx) * 180.0 / CV_PI;

    cv::Point2f center(
        static_cast<float>(face_crop.cols) / 2.0f, static_cast<float>(face_crop.rows) / 2.0f
    );

    cv::Mat rotation = cv::getRotationMatrix2D(center, angle, 1.0);
    cv::Mat rotated;

    cv::warpAffine(
        face_crop, rotated, rotation, face_crop.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT
    );

    return rotated;
}

auto MTCNNCropTransform::resizeAndPad(
    const cv::Mat& image
) const -> cv::Mat
{
    const int w = image.cols;
    const int h = image.rows;

    const float scale =
        static_cast<float>(imageDetectionSize_) / static_cast<float>(std::max(w, h));

    const int new_w = static_cast<int>(static_cast<float>(w) * scale);
    const int new_h = static_cast<int>(static_cast<float>(h) * scale);

    cv::Mat resized;

    cv::resize(image, resized, cv::Size(new_w, new_h), 0.0, 0.0, cv::INTER_LINEAR);

    if (new_w == imageDetectionSize_ && new_h == imageDetectionSize_)
    {
        return resized;
    }

    cv::Mat result(imageDetectionSize_, imageDetectionSize_, CV_8UC3, cv::Scalar(0, 0, 0));

    const int paste_x = (imageDetectionSize_ - new_w) / 2;
    const int paste_y = (imageDetectionSize_ - new_h) / 2;

    resized.copyTo(result(cv::Rect(paste_x, paste_y, new_w, new_h)));

    return result;
}

auto MTCNNCropTransform::operator()(
    const cv::Mat& image
) -> cv::Mat
{
    try
    {
        cv::Mat tmp = cv::imread("test.jpg", cv::IMREAD_COLOR);

        if (tmp.empty())
        {
            throw std::runtime_error("Failed to load test.jpg");
        }

        auto detection = detector_->detect(tmp);

        if (!detection.has_value())
        {
            lastFaceBox_.reset();

            cv::Mat result;

            if (tmp.channels() == 3)
            {
                result = tmp.clone();
            }
            else if (tmp.channels() == 4)
            {
                cv::cvtColor(tmp, result, cv::COLOR_BGRA2BGR);
            }
            else
            {
                cv::cvtColor(tmp, result, cv::COLOR_GRAY2BGR);
            }

            return result;
        }

        lastFaceBox_ = detection->box;

        cv::Mat face_crop = alignAndCrop(image, detection->box, detection->landmarks);

        return resizeAndPad(face_crop);
    }
    catch (const std::exception&)
    {
        lastFaceBox_.reset();

        cv::Mat result;

        if (image.channels() == 3)
        {
            result = image.clone();
        }
        else if (image.channels() == 4)
        {
            cv::cvtColor(image, result, cv::COLOR_BGRA2BGR);
        }
        else
        {
            cv::cvtColor(image, result, cv::COLOR_GRAY2BGR);
        }

        return result;
    }
}
