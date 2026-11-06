#include "ATen/core/ATen_fwd.h"
#include "Emotions/ResNet.hpp"
#include "torch/csrc/autograd/generated/variable_factories.h"
#include "torch/nn/modules/batchnorm.h"
#include "torch/nn/modules/container/sequential.h"
#include "torch/nn/modules/conv.h"
#include "torch/nn/options/conv.h"

#include <gtest/gtest.h>

TEST(
    ResNetTest, BasicBlockPreservesShapeWhenDimensionsMatch
)
{
    BasicBlock block(64, 64);
    block->eval();

    const auto input = torch::randn({2, 64, 8, 8});
    const auto output = block->forward(input);

    EXPECT_EQ(output.sizes(), input.sizes());
}

TEST(
    ResNetTest, BasicBlockDownsamplesWhenChannelsAndStrideChange
)
{
    torch::nn::Sequential downsample;
    downsample->push_back(
        torch::nn::Conv2d(torch::nn::Conv2dOptions(64, 128, 1).stride(2).bias(false))
    );
    downsample->push_back(torch::nn::BatchNorm2d(128));

    BasicBlock block(64, 128, 2, downsample);
    block->eval();

    const auto input = torch::randn({2, 64, 8, 8});
    const auto output = block->forward(input);

    EXPECT_EQ(output.sizes(), torch::IntArrayRef({2, 128, 4, 4}));
}

TEST(
    ResNetTest, ResNet34ProducesFeatureVector
)
{
    ResNet model = resnet34();
    model->eval();

    const auto input = torch::zeros({1, 3, 32, 32});
    const auto output = model->forward(input);

    EXPECT_EQ(output.sizes(), torch::IntArrayRef({1, ResNetImpl::kFeatureCount}));
}
