#ifndef FACECROP_HPP_
#define FACECROP_HPP_

#include "FaceDetector.hpp"

#include <array>
#include <memory>
#include <opencv2/opencv.hpp>
#include <optional>
#include <vector>

class MTCNNCropTransform
{
    public:

        MTCNNCropTransform(
            int image_detection_size, std::shared_ptr<IFaceDetector> detector, float margin = 0.1f
        );

        auto operator()(const cv::Mat& image) -> cv::Mat;

        [[nodiscard]] auto getLastFaceBox() const -> std::optional<cv::Rect2f>;

        auto operator()(const std::vector<cv::Mat>& images) -> std::vector<cv::Mat>;

    private:

        [[nodiscard]] auto alignAndCrop(
            const cv::Mat& image, const cv::Rect2f& box, const std::array<cv::Point2f, 5>& landmarks
        ) const -> cv::Mat;
        [[nodiscard]] auto resizeAndPad(const cv::Mat& image) const -> cv::Mat;

        int imageDetectionSize_;
        float margin_;

        std::shared_ptr<IFaceDetector> detector_;

        std::optional<cv::Rect2f> lastFaceBox_;
};

#endif /* !FACECROP_HPP_ */
