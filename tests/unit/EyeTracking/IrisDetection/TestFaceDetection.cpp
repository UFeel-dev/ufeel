#include "EyeTracking/IrisDetection/FaceDetection.hpp"

#include <cmath>
#include <gtest/gtest.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <string>
#include <vector>

class TestFaceDetection : public ::testing::Test
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

        detector_ = std::make_unique<my::FaceDetection>(modelDir_);
    }

    auto createImage(
        int width = 640, int height = 480, int type = CV_8UC3
    ) const -> cv::Mat
    {
        return cv::Mat(height, width, type, cv::Scalar(100, 150, 200, 255));
    }

    std::string modelDir_;
    std::unique_ptr<my::FaceDetection> detector_;
};

TEST_F(
    TestFaceDetection, ConstructorSucceedsWithValidModel
)
{
    EXPECT_NE(detector_, nullptr);
}

TEST_F(
    TestFaceDetection, LoadsThreeChannelImage
)
{
    auto const image = createImage();

    EXPECT_NO_THROW(detector_->loadImageToInput(image, 0));
}

TEST_F(
    TestFaceDetection, LoadsFourChannelImage
)
{
    auto const image = createImage(640, 480, CV_8UC4);

    EXPECT_NO_THROW(detector_->loadImageToInput(image, 0));
}

TEST_F(
    TestFaceDetection, StoresOriginalImage
)
{
    auto const image = createImage();

    detector_->loadImageToInput(image, 0);

    auto const original = detector_->getOriginalImage();

    EXPECT_FALSE(original.empty());
    EXPECT_EQ(original.size(), image.size());
    EXPECT_EQ(original.type(), image.type());
}

TEST_F(
    TestFaceDetection, StoresOriginalImagePixels
)
{
    auto const image = createImage();

    detector_->loadImageToInput(image, 0);

    auto const original = detector_->getOriginalImage();

    ASSERT_FALSE(original.empty());

    EXPECT_EQ(original.at<cv::Vec3b>(0, 0), image.at<cv::Vec3b>(0, 0));
}

TEST_F(
    TestFaceDetection, ThrowsWhenLoadingEmptyImage
)
{
    cv::Mat const image;

    EXPECT_THROW(detector_->loadImageToInput(image, 0), std::invalid_argument);
}

TEST_F(
    TestFaceDetection, ThrowsWhenLoadingUnsupportedImageType
)
{
    cv::Mat const image(480, 640, CV_8UC1);

    EXPECT_THROW(detector_->loadImageToInput(image, 0), std::invalid_argument);
}
