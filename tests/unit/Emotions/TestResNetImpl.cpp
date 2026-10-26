#include "Emotions/ResNetImpl.hpp"

#include <cstdint>
#include <gtest/gtest.h>
#include <torch/optim/sgd.h>
#include <torch/torch.h>
#include <vector>

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
