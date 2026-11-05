#include "EyeTracking/IrisDetection/ModelLoader.hpp"

#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

class TestModelLoader : public ::testing::Test
{
protected:

    void SetUp() override
    {
#ifndef MODEL_LOADER_TEST_MODEL_PATH
        FAIL() << "MODEL_LOADER_TEST_MODEL_PATH is not defined.";
        return;
#else
        modelPath_ = MODEL_LOADER_TEST_MODEL_PATH;
#endif

        loader_ = std::make_unique<my::ModelLoader>(modelPath_);
    }

    auto createImage(
        int channels = 3
    ) const -> cv::Mat
    {
        auto const shape = loader_->getInputShape();

        if (shape.size() != 4 || shape.at(1) <= 0 || shape.at(2) <= 0 || shape.at(3) != 3)
        {
            return {};
        }

        int const type = channels == 4 ? CV_8UC4 : CV_8UC3;

        return cv::Mat(shape.at(1), shape.at(2), type, cv::Scalar::all(128));
    }

    std::string modelPath_;
    std::unique_ptr<my::ModelLoader> loader_;
};

TEST_F(
    TestModelLoader, ConstructorSucceedsWithValidModel
)
{
    EXPECT_NE(loader_, nullptr);
}

TEST(
    ModelLoaderConstructor, ThrowsWhenModelDoesNotExist
)
{
    EXPECT_THROW(my::ModelLoader loader("invalid_model_path.tflite"), std::runtime_error);
}

TEST_F(
    TestModelLoader, ReturnsInputShapeForValidIndex
)
{
    auto const shape = loader_->getInputShape();

    EXPECT_FALSE(shape.empty());
}

TEST_F(
    TestModelLoader, ReturnsEmptyInputShapeForInvalidIndex
)
{
    EXPECT_TRUE(loader_->getInputShape(-1).empty());
    EXPECT_TRUE(loader_->getInputShape(999).empty());
}

TEST_F(
    TestModelLoader, ReturnsInputDataForValidIndex
)
{
    EXPECT_NE(loader_->getInputData(), nullptr);
}

TEST_F(
    TestModelLoader, ReturnsNullInputDataForInvalidIndex
)
{
    EXPECT_EQ(loader_->getInputData(-1), nullptr);
    EXPECT_EQ(loader_->getInputData(999), nullptr);
}

TEST_F(
    TestModelLoader, ReturnsInputSizeForValidIndex
)
{
    EXPECT_GT(loader_->getInputSize(), 0);
}

TEST_F(
    TestModelLoader, ReturnsZeroInputSizeForInvalidIndex
)
{
    EXPECT_EQ(loader_->getInputSize(-1), 0);
    EXPECT_EQ(loader_->getInputSize(999), 0);
}

TEST_F(
    TestModelLoader, ReturnsNumberOfInputs
)
{
    EXPECT_GT(loader_->getNumberOfInputs(), 0);
}
