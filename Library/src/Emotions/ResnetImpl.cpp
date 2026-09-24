/*
** EPITECH PROJECT, 2026
** ufeel
** File description:
** ResnetImpl
*/

#include "Emotions/ResNetImpl.hpp"
#include <torch/torch.h>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

BasicBlockImpl::BasicBlockImpl(int64_t inplanes, int64_t planes, int64_t stride, torch::nn::Sequential downsample_) : downsample(downsample_)
{
    conv1 = register_module("conv1", torch::nn::Conv2d(torch::nn::Conv2dOptions(inplanes, planes, 3).stride(stride).padding(1).bias(false)));
    bn1 = register_module("bn1", torch::nn::BatchNorm2d(planes));
    conv2 = register_module("conv2", torch::nn::Conv2d(torch::nn::Conv2dOptions(planes, planes, 3).stride(1).padding(1).bias(false)));
    bn2 = register_module("bn2", torch::nn::BatchNorm2d(planes));

    if (downsample)
    {
        register_module("downsample", downsample);
    }
}

torch::Tensor BasicBlockImpl::forward(torch::Tensor x)
{
    auto identity = x;

    auto out = conv1->forward(x);
    out = bn1->forward(out);
    out = torch::relu(out);

    out = conv2->forward(out);
    out = bn2->forward(out);

    if (downsample)
    {
        identity = downsample->forward(x);
    }

    out += identity;
    out = torch::relu(out);

    return out;
}

ResNetImpl::ResNetImpl(const std::vector<int64_t>& layers, int64_t num_classes)
{
    conv1 = register_module("conv1", torch::nn::Conv2d(torch::nn::Conv2dOptions(3, 64, 7).stride(2).padding(3).bias(false)));
    bn1 = register_module("bn1", torch::nn::BatchNorm2d(64));

    layer1 = register_module("layer1", make_layer(64, layers[0], 1));
    layer2 = register_module("layer2", make_layer(128, layers[1], 2));
    layer3 = register_module("layer3", make_layer(256, layers[2], 2));
    layer4 = register_module("layer4", make_layer(512, layers[3], 2));

    fc = register_module("fc", torch::nn::Linear(512, num_classes));
}

torch::nn::Sequential ResNetImpl::make_layer(int64_t planes, int64_t blocks, int64_t stride)
{
    torch::nn::Sequential layers;

    if (stride != 1 || inplanes != planes)
    {
        torch::nn::Sequential downsample;

        downsample->push_back(torch::nn::Conv2d(torch::nn::Conv2dOptions(inplanes, planes, 1).stride(stride).bias(false)));
        downsample->push_back(torch::nn::BatchNorm2d(planes));
        layers->push_back(BasicBlock(inplanes, planes, stride, downsample));

        inplanes = planes;
    }
    else
    {
        layers->push_back(BasicBlock(inplanes, planes));
    }

    for (int64_t i = 1; i < blocks; ++i)
    {
        layers->push_back(BasicBlock(inplanes, planes));
    }

    return layers;
}

torch::Tensor ResNetImpl::forward(torch::Tensor x)
{
    x = conv1->forward(x);
    x = bn1->forward(x);
    x = torch::relu(x);

    x = torch::max_pool2d(x, 3, 2, 1);

    x = layer1->forward(x);
    x = layer2->forward(x);
    x = layer3->forward(x);
    x = layer4->forward(x);

    x = torch::adaptive_avg_pool2d(x, {1, 1});
    x = torch::flatten(x, 1);
    x = fc->forward(x);

    return x;
}

ResNet resnet34()
{
    return ResNet({3, 4, 6, 3}, 1000);
}