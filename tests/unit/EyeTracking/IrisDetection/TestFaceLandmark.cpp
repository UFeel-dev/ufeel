#include "EyeTracking/IrisDetection/FaceLandmark.hpp"

#include <cmath>
#include <gtest/gtest.h>
#include <memory>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <string>
#include <vector>

class TestFaceLandmark : public ::testing::Test
{
protected:

    void SetUp() override
    {
#ifndef MODEL_DIR_PATH
        FAIL() << "MODEL_DIR_PATH is not defined.";
        return;
#else
        modelDir_ = MODEL_DIR_PATH;
#endif

        landmark_ = std::make_unique<my::FaceLandmark>(modelDir_);
    }

    auto createImage(
        int width = 640, int height = 480
    ) const -> cv::Mat
    {
        return cv::Mat(height, width, CV_8UC3, cv::Scalar(100, 150, 200));
    }

    auto createImageWithFace() -> bool
    {
        auto const image = createImage();

        landmark_->loadImageToInput(image, 0);
        landmark_->runInference();

        return !landmark_->getFaceRoi().empty();
    }

    std::string modelDir_;
    std::unique_ptr<my::FaceLandmark> landmark_;
};

TEST_F(
    TestFaceLandmark, ConstructorSucceedsWithValidModels
)
{
    EXPECT_NE(landmark_, nullptr);
}

TEST_F(
    TestFaceLandmark, LoadsThreeChannelImage
)
{
    auto const image = createImage();

    EXPECT_NO_THROW(landmark_->loadImageToInput(image, 0));
}

TEST_F(
    TestFaceLandmark, StoresOriginalImage
)
{
    auto const image = createImage();

    landmark_->loadImageToInput(image, 0);

    auto const original = landmark_->getOriginalImage();

    EXPECT_FALSE(original.empty());
    EXPECT_EQ(original.size(), image.size());
    EXPECT_EQ(original.type(), image.type());
}

TEST_F(
    TestFaceLandmark, ThrowsWhenLoadingEmptyImage
)
{
    cv::Mat const image;

    EXPECT_THROW(landmark_->loadImageToInput(image, 0), std::invalid_argument);
}

TEST_F(
    TestFaceLandmark, ThrowsWhenLoadingUnsupportedImageType
)
{
    cv::Mat const image(480, 640, CV_8UC1);

    EXPECT_THROW(landmark_->loadImageToInput(image, 0), std::invalid_argument);
}
