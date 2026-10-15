
#ifndef FACEDETECTOR_HPP_
#define FACEDETECTOR_HPP_

#include <array>
#include <memory>
#include <opencv2/opencv.hpp>
#include <optional>
#include <vector>

struct FaceDetection
{
        cv::Rect2f box{};

        std::array<cv::Point2f, 5> landmarks{};
};

class IFaceDetector
{
    protected:

        IFaceDetector() = default;

    public:

        virtual ~IFaceDetector() = default;
        virtual auto detect(const cv::Mat& image) -> std::optional<FaceDetection> = 0;

        IFaceDetector(const IFaceDetector&) = delete;
        auto operator=(const IFaceDetector&) -> IFaceDetector& = delete;

        IFaceDetector(IFaceDetector&&) = delete;
        auto operator=(IFaceDetector&&) -> IFaceDetector& = delete;
};

class YuNetFaceDetector : public IFaceDetector
{
    public:

        explicit YuNetFaceDetector(
            const std::string& model_path,
            float score_threshold = 0.9f,
            float nms_threshold = 0.3f,
            int top_k = 5000
        );

        auto detect(const cv::Mat& image) -> std::optional<FaceDetection> override;

    private:

        cv::Ptr<cv::FaceDetectorYN> detector_;
};

#endif /* !FACEDETECTOR_HPP_ */
