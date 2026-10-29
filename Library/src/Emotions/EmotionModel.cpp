#include "Emotions/EmotionModel.hpp"

#include "ATen/ops/cat.h"
#include "Emotions/EmotionLabels.hpp"
#include "Emotions/ResNet.hpp"
#include "torch/nn/module.h"
#include "torch/nn/modules/container/moduledict.h"
#include "torch/nn/modules/linear.h"
#include "torch/optim/optimizer.h"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

BinaryHeadImpl::BinaryHeadImpl(
    int64_t inFeatures
)
{
    linear_ = register_module("linear", torch::nn::Linear(inFeatures, 1));
}

auto BinaryHeadImpl::forward(
    const torch::Tensor& x
) -> torch::Tensor
{
    return linear_->forward(x);
}

EmotionModelImpl::EmotionModelImpl(
    const std::string& backboneName
)
{
    if (backboneName != "resnet34")
    {
        throw std::invalid_argument("Unsupported backbone: " + backboneName);
    }

    backbone_ = register_module("backbone", resnet34());

    std::vector<std::pair<std::string, std::shared_ptr<torch::nn::Module>>> modules;
    modules.reserve(kEmotionLabels.size());
    for (const auto label : kEmotionLabels)
    {
        modules.emplace_back(label, BinaryHead(ResNetImpl::kFeatureCount).ptr());
    }
    heads_ = register_module("heads", torch::nn::ModuleDict(modules));
}

auto EmotionModelImpl::forward(
    const torch::Tensor& x
) -> torch::Tensor
{
    const auto features = backbone_->forward(x);

    std::vector<torch::Tensor> logits;
    logits.reserve(kEmotionLabels.size());

    for (const auto label : kEmotionLabels)
    {
        logits.push_back(heads_->at<BinaryHeadImpl>(std::string(label)).forward(features));
    }

    return torch::cat(logits, 1);
}
