#include "Emotions/Inference/InferencePipeline.hpp"

#include "ATen/ops/sigmoid.h"
#include "Emotions/EmotionLabels.hpp"
#include "Emotions/EmotionModel.hpp"
#include "Emotions/Inference/FaceCrop.hpp"
#include "c10/core/Device.h"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/imgproc.hpp"
#include "torch/csrc/autograd/generated/variable_factories.h"
#include "torch/headeronly/core/DeviceType.h"
#include "torch/optim/optimizer.h"

#include <algorithm>
#include <torch/types.h>
#include <torch/utils.h>
#include <utility>

InferencePipeline::InferencePipeline(
    EmotionModel model, torch::Device device, FaceCrop faceCrop
)
    : model_(std::move(model))
    , device_(device)
    , faceCrop_(std::move(faceCrop))
{
    model_->to(device_);
    model_->eval();
}

auto InferencePipeline::preprocess(
    const cv::Mat& image
) -> torch::Tensor
{
    cv::Mat rgb;
    cv::cvtColor(faceCrop_(image), rgb, cv::COLOR_BGR2RGB);
    rgb.convertTo(rgb, CV_32FC3, 1.0 / 255.0);

    const auto mean = torch::tensor({0.485F, 0.456F, 0.406F}).view({1, 3, 1, 1});
    const auto std = torch::tensor({0.229F, 0.224F, 0.225F}).view({1, 3, 1, 1});

    const auto pixels = torch::from_blob(rgb.data, {1, rgb.rows, rgb.cols, 3}, torch::kFloat32)
                            .permute({0, 3, 1, 2});

    return ((pixels - mean) / std).contiguous();
}

auto InferencePipeline::predict(
    const cv::Mat& image
) -> EmotionScores
{
    const torch::NoGradGuard noGrad;

    const auto probabilities =
        torch::sigmoid(model_->forward(preprocess(image).to(device_))).to(torch::kCPU).contiguous();

    EmotionScores scores{};
    std::copy_n(probabilities.data_ptr<float>(), scores.size(), scores.begin());
    return scores;
}
