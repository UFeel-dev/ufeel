#include "ATen/core/ATen_fwd.h"
#include "Emotions/EmotionLabels.hpp"
#include "Emotions/EmotionModel.hpp"
#include "torch/csrc/autograd/generated/variable_factories.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <stdexcept>

TEST(
    EmotionModelTest, BinaryHeadProducesOneLogitPerInput
)
{
    BinaryHead head(512);

    const auto input = torch::randn({2, 512});
    const auto output = head->forward(input);

    EXPECT_EQ(output.sizes(), torch::IntArrayRef({2, 1}));
}

TEST(
    EmotionModelTest, ForwardProducesOneLogitPerEmotion
)
{
    EmotionModel model("resnet34");
    model->eval();

    const auto input = torch::zeros({1, 3, 32, 32});
    const auto output = model->forward(input);

    EXPECT_EQ(output.sizes(), torch::IntArrayRef({1, static_cast<int64_t>(kEmotionLabels.size())}));
}

TEST(
    EmotionModelTest, RejectsUnsupportedBackbone
)
{
    EXPECT_THROW(EmotionModel("unsupported"), std::invalid_argument);
}
