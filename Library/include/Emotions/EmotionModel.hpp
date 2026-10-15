#ifndef EMOTIONMODEL_HPP_
#define EMOTIONMODEL_HPP_

#include "ResNet.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <torch/torch.h>
#include <unordered_map>

struct BinaryHeadImpl : torch::nn::Module
{
        torch::nn::Linear linear{nullptr};

        explicit BinaryHeadImpl(int64_t in_features);

        auto forward(torch::Tensor x) -> torch::Tensor;
};

TORCH_MODULE(BinaryHead);

struct EmotionHeadsImpl : torch::nn::Module
{
        BinaryHead angry{nullptr};
        BinaryHead contemptuous{nullptr};
        BinaryHead disgusted{nullptr};
        BinaryHead fearful{nullptr};
        BinaryHead happy{nullptr};
        BinaryHead neutral{nullptr};
        BinaryHead sad{nullptr};
        BinaryHead surprised{nullptr};

        explicit EmotionHeadsImpl(
            int64_t in_features
        )
        {
            angry = register_module("angry", BinaryHead(in_features));

            contemptuous = register_module("contemptuous", BinaryHead(in_features));

            disgusted = register_module("disgusted", BinaryHead(in_features));

            fearful = register_module("fearful", BinaryHead(in_features));

            happy = register_module("happy", BinaryHead(in_features));

            neutral = register_module("neutral", BinaryHead(in_features));

            sad = register_module("sad", BinaryHead(in_features));

            surprised = register_module("surprised", BinaryHead(in_features));
        }
};

TORCH_MODULE(EmotionHeads);

struct EmotionModelImpl : torch::nn::Module
{
        ResNet backbone{nullptr};
        EmotionHeads heads{nullptr};

        explicit EmotionModelImpl(const std::string& backbone_name);

        auto extract_features(torch::Tensor x) -> torch::Tensor;

        auto forward(torch::Tensor x) -> std::unordered_map<std::string, torch::Tensor>;
};

TORCH_MODULE(EmotionModel);

#endif /* !EMOTIONMODEL_HPP_ */
