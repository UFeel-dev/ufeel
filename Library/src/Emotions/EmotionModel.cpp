#include "Emotions/EmotionModel.hpp"

#include <stdexcept>
#include <string>
#include <torch/torch.h>
#include <unordered_map>
#include <utility>
#include <vector>

BinaryHeadImpl::BinaryHeadImpl(
    int64_t in_features
)
{
    linear = register_module("linear", torch::nn::Linear(in_features, 1));
}

auto BinaryHeadImpl::forward(
    torch::Tensor x
) -> torch::Tensor
{
    return linear->forward(x);
}

EmotionModelImpl::EmotionModelImpl(
    const std::string& backbone_name
)
{
    int64_t in_features = 0;

    if (backbone_name == "resnet34")
    {
        backbone = resnet34();
        in_features = 512;
    }
    else
    {
        throw std::runtime_error("Unsupported backbone: " + backbone_name);
    }

    register_module("backbone", backbone);

    heads = EmotionHeads(in_features);
    register_module("heads", heads);
}

auto EmotionModelImpl::extract_features(
    torch::Tensor x
) -> torch::Tensor
{
    x = backbone->conv1->forward(x);
    x = backbone->bn1->forward(x);
    x = torch::relu(x);

    x = torch::max_pool2d(x, 3, 2, 1);

    x = backbone->layer1->forward(x);
    x = backbone->layer2->forward(x);
    x = backbone->layer3->forward(x);
    x = backbone->layer4->forward(x);

    x = torch::adaptive_avg_pool2d(x, {1, 1});

    x = torch::flatten(x, 1);

    return x;
}

auto EmotionModelImpl::forward(
    torch::Tensor x
) -> std::unordered_map<std::string, torch::Tensor>
{
    auto features = extract_features(std::move(x));

    return {
        {"angry", heads->angry->forward(features).squeeze(1)},
        {"contemptuous", heads->contemptuous->forward(features).squeeze(1)},
        {"disgusted", heads->disgusted->forward(features).squeeze(1)},
        {"fearful", heads->fearful->forward(features).squeeze(1)},
        {"happy", heads->happy->forward(features).squeeze(1)},
        {"neutral", heads->neutral->forward(features).squeeze(1)},
        {"sad", heads->sad->forward(features).squeeze(1)},
        {"surprised", heads->surprised->forward(features).squeeze(1)}};
}
