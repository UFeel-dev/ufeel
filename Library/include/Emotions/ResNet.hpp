#ifndef EMOTIONS_RESNET_HPP
#define EMOTIONS_RESNET_HPP

#include "torch/nn/module.h"
#include "torch/nn/modules/batchnorm.h"
#include "torch/nn/modules/container/sequential.h"
#include "torch/nn/modules/conv.h"
#include "torch/nn/pimpl.h"
#include "torch/optim/optimizer.h"

#include <array>
#include <cstdint>

struct BasicBlockImpl : torch::nn::Module
{
    BasicBlockImpl(
        int64_t inPlanes,
        int64_t planes,
        int64_t stride = 1,
        torch::nn::Sequential downsampleLayers = nullptr
    );

    auto forward(const torch::Tensor& x) -> torch::Tensor;

private:

    torch::nn::Conv2d conv1_{nullptr};
    torch::nn::BatchNorm2d bn1_{nullptr};
    torch::nn::Conv2d conv2_{nullptr};
    torch::nn::BatchNorm2d bn2_{nullptr};
    torch::nn::Sequential downsample_{nullptr};
};

TORCH_MODULE(BasicBlock);

struct ResNetImpl : torch::nn::Module
{
    static constexpr int64_t kFeatureCount = 512;

    explicit ResNetImpl(const std::array<int64_t, 4>& blocksPerStage);

    auto forward(torch::Tensor x) -> torch::Tensor;

private:

    auto makeStage(int64_t planes, int64_t blocks, int64_t stride) -> torch::nn::Sequential;

    int64_t inPlanes_ = 64;

    torch::nn::Conv2d conv1_{nullptr};
    torch::nn::BatchNorm2d bn1_{nullptr};
    torch::nn::Sequential layer1_{nullptr};
    torch::nn::Sequential layer2_{nullptr};
    torch::nn::Sequential layer3_{nullptr};
    torch::nn::Sequential layer4_{nullptr};
};

TORCH_MODULE(ResNet);

auto resnet34() -> ResNet;

#endif // EMOTIONS_RESNET_HPP
