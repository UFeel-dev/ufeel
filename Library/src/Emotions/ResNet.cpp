#include "Emotions/ResNet.hpp"

#include "ATen/ops/adaptive_avg_pool2d.h"
#include "ATen/ops/flatten.h"
#include "ATen/ops/max_pool2d.h"
#include "ATen/ops/relu.h"
#include "torch/nn/modules/batchnorm.h"
#include "torch/nn/modules/container/sequential.h"
#include "torch/nn/modules/conv.h"
#include "torch/nn/options/conv.h"
#include "torch/optim/optimizer.h"

#include <array>
#include <cstdint>
#include <utility>

BasicBlockImpl::BasicBlockImpl(
    int64_t inPlanes, int64_t planes, int64_t stride, torch::nn::Sequential downsampleLayers
)
{
    conv1_ = register_module(
        "conv1",
        torch::nn::Conv2d(
            torch::nn::Conv2dOptions(inPlanes, planes, 3).stride(stride).padding(1).bias(false)
        )
    );
    bn1_ = register_module("bn1", torch::nn::BatchNorm2d(planes));
    conv2_ = register_module(
        "conv2",
        torch::nn::Conv2d(torch::nn::Conv2dOptions(planes, planes, 3).padding(1).bias(false))
    );
    bn2_ = register_module("bn2", torch::nn::BatchNorm2d(planes));

    if (!downsampleLayers.is_empty())
    {
        downsample_ = register_module("downsample", std::move(downsampleLayers));
    }
}

auto BasicBlockImpl::forward(
    const torch::Tensor& x
) -> torch::Tensor
{
    auto out = torch::relu(bn1_->forward(conv1_->forward(x)));
    out = bn2_->forward(conv2_->forward(out));

    const auto identity = downsample_.is_empty() ? x : downsample_->forward(x);

    return torch::relu(out + identity);
}

ResNetImpl::ResNetImpl(
    const std::array<int64_t, 4>& blocksPerStage
)
{
    conv1_ = register_module(
        "conv1",
        torch::nn::Conv2d(torch::nn::Conv2dOptions(3, 64, 7).stride(2).padding(3).bias(false))
    );
    bn1_ = register_module("bn1", torch::nn::BatchNorm2d(64));

    layer1_ = register_module("layer1", makeStage(64, blocksPerStage.at(0), 1));
    layer2_ = register_module("layer2", makeStage(128, blocksPerStage.at(1), 2));
    layer3_ = register_module("layer3", makeStage(256, blocksPerStage.at(2), 2));
    layer4_ = register_module("layer4", makeStage(kFeatureCount, blocksPerStage.at(3), 2));
}

auto ResNetImpl::makeStage(
    int64_t planes, int64_t blocks, int64_t stride
) -> torch::nn::Sequential
{
    torch::nn::Sequential downsample = nullptr;

    if (stride != 1 || inPlanes_ != planes)
    {
        downsample = torch::nn::Sequential(
            torch::nn::Conv2d(
                torch::nn::Conv2dOptions(inPlanes_, planes, 1).stride(stride).bias(false)
            ),
            torch::nn::BatchNorm2d(planes)
        );
    }

    torch::nn::Sequential stage;
    stage->push_back(BasicBlock(inPlanes_, planes, stride, downsample));
    inPlanes_ = planes;

    for (int64_t i = 1; i < blocks; ++i)
    {
        stage->push_back(BasicBlock(planes, planes));
    }

    return stage;
}

auto ResNetImpl::forward(
    torch::Tensor x
) -> torch::Tensor
{
    x = torch::relu(bn1_->forward(conv1_->forward(x)));
    x = torch::max_pool2d(x, {3, 3}, {2, 2}, {1, 1});

    x = layer1_->forward(x);
    x = layer2_->forward(x);
    x = layer3_->forward(x);
    x = layer4_->forward(x);

    return torch::flatten(torch::adaptive_avg_pool2d(x, {1, 1}), 1);
}

auto resnet34() -> ResNet
{
    return ResNet(std::array<int64_t, 4>{3, 4, 6, 3});
}
