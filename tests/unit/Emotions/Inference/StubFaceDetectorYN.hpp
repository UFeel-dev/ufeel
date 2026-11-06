#ifndef TESTS_UNIT_EMOTIONS_INFERENCE_STUBFACEDETECTORYN_HPP
#define TESTS_UNIT_EMOTIONS_INFERENCE_STUBFACEDETECTORYN_HPP

#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"
#include "opencv2/objdetect/face.hpp"

#include <utility>

class StubFaceDetectorYN final : public cv::FaceDetectorYN
{
public:

    explicit StubFaceDetectorYN(
        cv::Mat faces
    )
        : faces_(std::move(faces))
    {
    }

    void setInputSize(
        const cv::Size& size
    ) override
    {
        inputSize_ = size;
    }

    auto getInputSize() -> cv::Size override
    {
        return inputSize_;
    }

    void setScoreThreshold(
        float threshold
    ) override
    {
        static_cast<void>(threshold);
    }

    auto getScoreThreshold() -> float override
    {
        return 0.9F;
    }

    void setNMSThreshold(
        float threshold
    ) override
    {
        static_cast<void>(threshold);
    }

    auto getNMSThreshold() -> float override
    {
        return 0.3F;
    }

    void setTopK(
        int topK
    ) override
    {
        static_cast<void>(topK);
    }

    auto getTopK() -> int override
    {
        return 5000;
    }

    auto detect(
        cv::InputArray /*image*/, cv::OutputArray faces
    ) -> int override
    {
        faces_.copyTo(faces);
        return faces_.rows;
    }

private:

    cv::Mat faces_;
    cv::Size inputSize_;
};

#endif // TESTS_UNIT_EMOTIONS_INFERENCE_STUBFACEDETECTORYN_HPP
