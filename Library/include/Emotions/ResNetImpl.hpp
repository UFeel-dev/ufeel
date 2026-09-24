/*
** EPITECH PROJECT, 2026
** ufeel
** File description:
** ResNetImpl
*/

#ifndef RESNETIMPL_HPP_
    #define RESNETIMPL_HPP_

    #include <torch/torch.h>

    #include <cstdint>
    #include <vector>

struct BasicBlockImpl : torch::nn::Module
{
    static constexpr int64_t expansion = 1;

    torch::nn::Conv2d conv1{nullptr};
    torch::nn::BatchNorm2d bn1{nullptr};

    torch::nn::Conv2d conv2{nullptr};
    torch::nn::BatchNorm2d bn2{nullptr};

    torch::nn::Sequential downsample{nullptr};

    BasicBlockImpl(
        int64_t inplanes,
        int64_t planes,
        int64_t stride = 1,
        torch::nn::Sequential downsample = nullptr);

    torch::Tensor forward(torch::Tensor x);
};

TORCH_MODULE(BasicBlock);

struct ResNetImpl : torch::nn::Module
{
    torch::nn::Conv2d conv1{nullptr};
    torch::nn::BatchNorm2d bn1{nullptr};

    torch::nn::Sequential layer1{nullptr};
    torch::nn::Sequential layer2{nullptr};
    torch::nn::Sequential layer3{nullptr};
    torch::nn::Sequential layer4{nullptr};

    torch::nn::Linear fc{nullptr};

    int64_t inplanes = 64;

    ResNetImpl(const std::vector<int64_t>& layers, int64_t num_classes = 1000);

    torch::nn::Sequential make_layer(int64_t planes, int64_t blocks,int64_t stride);
    torch::Tensor forward(torch::Tensor x);
};

TORCH_MODULE(ResNet);

ResNet resnet34();

#endif /* !RESNETIMPL_HPP_ */
