#include "../../../Library/src/Emotions/Model.cpp"

#include <gtest/gtest.h>
#include <torch/torch.h>

TEST(
    ModelTest, BinaryHeadProducesExpectedOutput
)
{
    BinaryHead head(512);

    auto input = torch::randn({2, 512});
    auto output = head->forward(input);

    EXPECT_EQ(output.sizes(), torch::IntArrayRef({2, 1}));
}
