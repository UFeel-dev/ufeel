#include "Emotions/ResNetImpl.hpp"

#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <torch/optim/sgd.h>
#include <torch/torch.h>
#include <vector>

TEST(
    BasicBlockTest, RegistersDownsampleModule
)
{
    torch::nn::Sequential downsample;
    downsample->push_back(
        torch::nn::Conv2d(torch::nn::Conv2dOptions(64, 128, 1).stride(2).bias(false))
    );
    downsample->push_back(torch::nn::BatchNorm2d(128));

    BasicBlockImpl block(64, 128, 2, downsample);

    ASSERT_TRUE(block.downsample);
}

TEST(
    BasicBlockTest, ForwardPreservesShapeWhenStrideIsOne
)
{
    BasicBlock block(64, 64);

    auto input = torch::randn({2, 64, 16, 16});
    auto output = block->forward(input);

    EXPECT_EQ(output.sizes(), input.sizes());
}

TEST(
    BasicBlockTest, ForwardChangesShapeWhenStrideIsTwo
)
{
    torch::nn::Sequential downsample;

    downsample->push_back(
        torch::nn::Conv2d(torch::nn::Conv2dOptions(64, 128, 1).stride(2).bias(false))
    );
    downsample->push_back(torch::nn::BatchNorm2d(128));

    BasicBlock block(64, 128, 2, downsample);

    auto input = torch::randn({2, 64, 16, 16});
    auto output = block->forward(input);

    EXPECT_EQ(output.sizes(), torch::IntArrayRef({2, 128, 8, 8}));
}

TEST(
    ResNetTest, ResNet34ProducesExpectedOutputShape
)
{
    auto model = resnet34();
    auto input = torch::randn({1, 3, 224, 224});

    auto output = model->forward(input);

    EXPECT_EQ(output.sizes(), torch::IntArrayRef({1, 1000}));
}

TEST(
    ResNetTest, MakeLayerUsesDownsampleWhenChannelsDifferAndStrideIsOne
)
{
    std::vector<int64_t> layers = {1, 1, 1, 1};
    ResNet model(layers, 1000);

    model->inplanes = 64;

    auto layer = model->makeLayer(128, 1, 1);

    auto input = torch::randn({2, 64, 16, 16});
    auto output = layer->forward(input);

    EXPECT_EQ(output.sizes(), torch::IntArrayRef({2, 128, 16, 16}));
}

TEST(
    ResNetTest, MakeLayerSupportsMultipleBlocks
)
{
    ResNet model = ResNet(std::make_shared<ResNetImpl>(std::vector<int64_t>{1, 1, 1, 1}, 1000));
    model->inplanes = 64;

    auto layer = model->makeLayer(64, 3, 1);

    auto input = torch::randn({2, 64, 16, 16});
    auto output = layer->forward(input);

    EXPECT_EQ(output.sizes(), input.sizes());
}
