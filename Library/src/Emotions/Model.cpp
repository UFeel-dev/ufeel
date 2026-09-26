// NOLINTBEGIN(misc-include-cleaner)
#include <torch/torch.h>
#include <stdexcept>
#include <string>
#include <unordered_map>
// #include <vector>
#include "Emotions/ResNetImpl.hpp"

struct BinaryHeadImpl : torch::nn::Module
{
    torch::nn::Linear linear{nullptr};

    explicit BinaryHeadImpl(int64_t in_features)
    {
        linear = register_module("linear", torch::nn::Linear(in_features, 1));
    }

    // NOLINT(performance-unnecessary-value-param)
    auto forward(torch::Tensor x) -> torch::Tensor
    {
        return linear->forward(x);
    }
};

TORCH_MODULE(BinaryHead);

struct EmotionModelImpl : torch::nn::Module
{
    ResNet backbone{nullptr};

    BinaryHead angry{nullptr};
    BinaryHead contemptuous{nullptr};
    BinaryHead disgusted{nullptr};
    BinaryHead fearful{nullptr};
    BinaryHead happy{nullptr};
    BinaryHead neutral{nullptr};
    BinaryHead sad{nullptr};
    BinaryHead surprised{nullptr};

    explicit EmotionModelImpl(const std::string& backbone_name)
    {
        int64_t in_features = 0;

        if (backbone_name == "resnet34")
        {
            backbone = resnet34();

            in_features = 512;
        }
        else
        {
            throw std::runtime_error(
                "Unsupported backbone: " + backbone_name);
        }

        register_module("backbone", backbone);

        angry = register_module("angry", BinaryHead(in_features));
        contemptuous = register_module("contemptuous", BinaryHead(in_features));
        disgusted = register_module("disgusted", BinaryHead(in_features));
        fearful = register_module("fearful", BinaryHead(in_features));
        happy = register_module("happy", BinaryHead(in_features));
        neutral = register_module("neutral", BinaryHead(in_features));
        sad = register_module("sad", BinaryHead(in_features));
        surprised = register_module("surprised", BinaryHead(in_features));
    }

    auto extract_features(torch::Tensor x) -> torch::Tensor
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

    auto forward(torch::Tensor x) -> std::unordered_map<std::string, torch::Tensor>
    {
        auto features = extract_features(std::move(x));

        return
        {
            {
                "angry", angry->forward(features).squeeze(1)
            },
            {
                "contemptuous", contemptuous->forward(features).squeeze(1)
            },
            {
                "disgusted", disgusted->forward(features).squeeze(1)
            },
            {
                "fearful", fearful->forward(features).squeeze(1)
            },
            {
                "happy", happy->forward(features).squeeze(1)
            },
            {
                "neutral", neutral->forward(features).squeeze(1)
            },
            {
                "sad", sad->forward(features).squeeze(1)
            },
            {
                "surprised", surprised->forward(features).squeeze(1)
            }
        };
    }
};

// NOLINTEND(misc-include-cleaner)