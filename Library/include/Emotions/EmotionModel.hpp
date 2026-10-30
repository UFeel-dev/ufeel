#ifndef EMOTIONS_EMOTIONMODEL_HPP
#define EMOTIONS_EMOTIONMODEL_HPP

#include "ResNet.hpp"
#include "torch/nn/module.h"
#include "torch/nn/modules/container/moduledict.h"
#include "torch/nn/modules/linear.h"
#include "torch/nn/pimpl.h"
#include "torch/optim/optimizer.h"

#include <cstdint>
#include <string>

struct BinaryHeadImpl : torch::nn::Module
{
    explicit BinaryHeadImpl(int64_t inFeatures);

    auto forward(const torch::Tensor& x) -> torch::Tensor;

private:

    torch::nn::Linear linear_{nullptr};
};

TORCH_MODULE(BinaryHead);

struct EmotionModelImpl : torch::nn::Module
{
    explicit EmotionModelImpl(const std::string& backboneName);

    auto forward(const torch::Tensor& x) -> torch::Tensor;

private:

    ResNet backbone_{nullptr};
    torch::nn::ModuleDict heads_{nullptr};
};

TORCH_MODULE(EmotionModel);

#endif // EMOTIONS_EMOTIONMODEL_HPP
