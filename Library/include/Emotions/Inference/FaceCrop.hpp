#ifndef EMOTIONS_INFERENCE_FACECROP_HPP
#define EMOTIONS_INFERENCE_FACECROP_HPP

#include "FaceDetector.hpp"
#include "opencv2/core/mat.hpp"

class FaceCrop
{
public:

    FaceCrop(FaceDetector detector, int size, float margin = 0.1F);

    [[nodiscard]] auto operator()(const cv::Mat& image) -> cv::Mat;

private:

    [[nodiscard]] auto resizeAndPad(const cv::Mat& image) const -> cv::Mat;

    FaceDetector detector_;
    int size_;
    float margin_;
};

#endif // EMOTIONS_INFERENCE_FACECROP_HPP
