#include "EyeTracking/IrisDetection/ModelLoader.hpp"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"

#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#ifndef MODEL_LOADER_TEST_MODEL_PATH
#define MODEL_LOADER_TEST_MODEL_PATH ""
#endif

namespace
{

class TestableModelLoader final : public my::ModelLoader
{
public:

    explicit TestableModelLoader(
        const std::string& modelPath
    )
        : my::ModelLoader(modelPath)
    {
    }

    std::vector<int> inputShape{1, 2, 2, 3};

    [[nodiscard]] auto getInputShape(
        int /*index*/
    ) const -> std::vector<int> override
    {
        return inputShape;
    }

    [[nodiscard]] auto getInputSize(
        int /*index*/
    ) const -> std::size_t override
    {
        return 1;
    }
};

class TestModelLoader : public ::testing::Test
{
protected:

    void SetUp() override
    {
        loader_ = std::make_unique<my::ModelLoader>(MODEL_LOADER_TEST_MODEL_PATH);
    }

    [[nodiscard]] auto createImage(
        int channels = 3
    ) const -> cv::Mat
    {
        auto const shape = loader_->getInputShape();

        if (shape.size() != 4 || shape.at(1) <= 0 || shape.at(2) <= 0 || shape.at(3) != 3)
        {
            return {};
        }

        int const type = channels == 4 ? CV_8UC4 : CV_8UC3;

        return {shape.at(1), shape.at(2), type, cv::Scalar::all(128)};
    }

    std::unique_ptr<my::ModelLoader> loader_;
};

} // namespace

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
    EXPECT_THROW(const my::ModelLoader loader("invalid_model_path.tflite"), std::runtime_error);
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

TEST_F(
    TestModelLoader, ReturnsOutputShapeForValidIndex
)
{
    auto const shape = loader_->getOutputShape();

    EXPECT_FALSE(shape.empty());
}

TEST_F(
    TestModelLoader, ReturnsEmptyOutputShapeForInvalidIndex
)
{
    EXPECT_TRUE(loader_->getOutputShape(-1).empty());
    EXPECT_TRUE(loader_->getOutputShape(999).empty());
}

TEST_F(
    TestModelLoader, ReturnsOutputDataForValidIndex
)
{
    EXPECT_NE(loader_->getOutputData(), nullptr);
}

TEST_F(
    TestModelLoader, ReturnsNullOutputDataForInvalidIndex
)
{
    EXPECT_EQ(loader_->getOutputData(-1), nullptr);
    EXPECT_EQ(loader_->getOutputData(999), nullptr);
}

TEST_F(
    TestModelLoader, ReturnsOutputSizeForValidIndex
)
{
    EXPECT_GT(loader_->getOutputSize(), 0);
}

TEST_F(
    TestModelLoader, ReturnsZeroOutputSizeForInvalidIndex
)
{
    EXPECT_EQ(loader_->getOutputSize(-1), 0);
    EXPECT_EQ(loader_->getOutputSize(999), 0);
}

TEST_F(
    TestModelLoader, ReturnsNumberOfOutputs
)
{
    EXPECT_GT(loader_->getNumberOfOutputs(), 0);
}

TEST_F(
    TestModelLoader, LoadsBytesToInput
)
{
    std::vector<std::uint8_t> data(loader_->getInputSize(), 0);

    EXPECT_NO_THROW(loader_->loadBytesToInput(data.data()));
}

TEST_F(
    TestModelLoader, ThrowsWhenLoadingNullBytes
)
{
    EXPECT_THROW(loader_->loadBytesToInput(nullptr), std::invalid_argument);
}

TEST_F(
    TestModelLoader, ThrowsWhenLoadingBytesWithInvalidNegativeIndex
)
{
    std::vector<std::uint8_t> data(loader_->getInputSize(), 0);

    EXPECT_THROW(loader_->loadBytesToInput(data.data(), -1), std::out_of_range);
}

TEST_F(
    TestModelLoader, ThrowsWhenLoadingBytesWithInvalidLargeIndex
)
{
    std::vector<std::uint8_t> data(loader_->getInputSize(), 0);

    EXPECT_THROW(loader_->loadBytesToInput(data.data(), 999), std::out_of_range);
}

TEST_F(
    TestModelLoader, LoadsBytesToEveryInput
)
{
    for (int index = 0; index < loader_->getNumberOfInputs(); ++index)
    {
        std::vector<std::uint8_t> data(loader_->getInputSize(index), 0);

        EXPECT_NO_THROW(loader_->loadBytesToInput(data.data(), index));
    }
}

TEST_F(
    TestModelLoader, LoadsThreeChannelImage
)
{
    auto const image = createImage(3);

    ASSERT_FALSE(image.empty());

    EXPECT_NO_THROW(loader_->loadImageToInput(image));
}

TEST_F(
    TestModelLoader, LoadsFourChannelImage
)
{
    auto const image = createImage(4);

    ASSERT_FALSE(image.empty());

    EXPECT_NO_THROW(loader_->loadImageToInput(image));
}

TEST_F(
    TestModelLoader, ThrowsWhenLoadingEmptyImage
)
{
    cv::Mat const image;

    EXPECT_THROW(loader_->loadImageToInput(image), std::invalid_argument);
}

TEST_F(
    TestModelLoader, ThrowsWhenLoadingImageWithUnsupportedType
)
{
    cv::Mat const image(128, 128, CV_8UC1);

    EXPECT_THROW(loader_->loadImageToInput(image), std::invalid_argument);
}

TEST_F(
    TestModelLoader, ThrowsWhenLoadingImageWithInvalidNegativeIndex
)
{
    auto const image = createImage();

    ASSERT_FALSE(image.empty());

    EXPECT_THROW(loader_->loadImageToInput(image, -1), std::out_of_range);
}

TEST_F(
    TestModelLoader, ThrowsWhenLoadingImageWithInvalidLargeIndex
)
{
    auto const image = createImage();

    ASSERT_FALSE(image.empty());

    EXPECT_THROW(loader_->loadImageToInput(image, 999), std::out_of_range);
}

TEST(
    ModelLoaderInjectionTest, RejectsPreprocessedImageWithUnexpectedSize
)
{
    TestableModelLoader loader(MODEL_LOADER_TEST_MODEL_PATH);
    cv::Mat const image(2, 2, CV_8UC3, cv::Scalar::all(128));

    EXPECT_THROW(loader.loadImageToInput(image), std::runtime_error);
}

TEST(
    ModelLoaderInjectionTest, RejectsUnsupportedInputShape
)
{
    TestableModelLoader loader(MODEL_LOADER_TEST_MODEL_PATH);
    loader.inputShape = {1, 0, 2, 3};
    cv::Mat const image(2, 2, CV_8UC3, cv::Scalar::all(128));

    EXPECT_THROW(loader.loadImageToInput(image), std::runtime_error);
}

TEST_F(
    TestModelLoader, RejectsInferenceBeforeAllInputsAreLoaded
)
{
    EXPECT_THROW(loader_->runInference(), std::runtime_error);
}

TEST_F(
    TestModelLoader, RunsInferenceAfterLoadingEveryInput
)
{
    for (int index = 0; index < loader_->getNumberOfInputs(); ++index)
    {
        std::vector<std::uint8_t> data(loader_->getInputSize(index), 0);
        loader_->loadBytesToInput(data.data(), index);
    }

    EXPECT_NO_THROW(loader_->runInference());
}

TEST_F(
    TestModelLoader, RequiresInputsToBeLoadedAgainAfterInference
)
{
    std::vector<std::uint8_t> data(loader_->getInputSize(), 0);
    loader_->loadBytesToInput(data.data());
    ASSERT_NO_THROW(loader_->runInference());

    EXPECT_THROW(loader_->runInference(), std::runtime_error);
}
