#ifndef EMOTIONS_INFERENCE_INFERENCEPIPELINE_HPP
#define EMOTIONS_INFERENCE_INFERENCEPIPELINE_HPP

#include "Emotions/EmotionLabels.hpp"
#include "Emotions/EmotionModel.hpp"
#include "FaceCrop.hpp"
#include "c10/core/Device.h"
#include "opencv2/core/mat.hpp"
#include "torch/optim/optimizer.h"

class InferencePipeline
{
public:

    InferencePipeline(EmotionModel model, torch::Device device, FaceCrop faceCrop);

    [[nodiscard]] auto predict(const cv::Mat& image) -> EmotionScores;

private:

    [[nodiscard]] auto preprocess(const cv::Mat& image) -> torch::Tensor;

    EmotionModel model_;
    torch::Device device_;
    FaceCrop faceCrop_;
};

#endif // EMOTIONS_INFERENCE_INFERENCEPIPELINE_HPP
