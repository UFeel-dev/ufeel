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

TEST_F(
    TestFaceLandmark, ThrowsWhenRunningInferenceWithoutImage
)
{
    EXPECT_THROW(landmark_->runInference(), std::runtime_error);
}

TEST_F(
    TestFaceLandmark, RunsInferenceAfterLoadingImage
)
{
    auto const image = createImage();

    landmark_->loadImageToInput(image, 0);

    EXPECT_NO_THROW(landmark_->runInference());
}

TEST_F(
    TestFaceLandmark, ReturnsOriginalImageAfterInference
)
{
    auto const image = createImage();

    landmark_->loadImageToInput(image, 0);
    landmark_->runInference();

    auto const original = landmark_->getOriginalImage();

    EXPECT_FALSE(original.empty());
    EXPECT_EQ(original.size(), image.size());
}

TEST_F(
    TestFaceLandmark, ReturnsZeroPointForNegativeLandmarkIndex
)
{
    EXPECT_EQ(landmark_->getFaceLandmarkAt(-1), cv::Point());
}

TEST_F(
    TestFaceLandmark, ReturnsZeroPointForLandmarkIndexAboveRange
)
{
    EXPECT_EQ(landmark_->getFaceLandmarkAt(468), cv::Point());
}

TEST_F(
    TestFaceLandmark, ReturnsZeroPointForLargeLandmarkIndex
)
{
    EXPECT_EQ(landmark_->getFaceLandmarkAt(999), cv::Point());
}

TEST_F(
    TestFaceLandmark, ReturnsZeroPoint3DForNegativeLandmarkIndex
)
{
    EXPECT_EQ(landmark_->getFaceLandmark3DAt(-1), cv::Point3f());
}

TEST_F(
    TestFaceLandmark, ReturnsZeroPoint3DForLandmarkIndexAboveRange
)
{
    EXPECT_EQ(landmark_->getFaceLandmark3DAt(468), cv::Point3f());
}

TEST_F(
    TestFaceLandmark, ReturnsZeroPoint3DForLargeLandmarkIndex
)
{
    EXPECT_EQ(landmark_->getFaceLandmark3DAt(999), cv::Point3f());
}

TEST_F(
    TestFaceLandmark, ReturnsZeroPointWhenNoFaceRoiExists
)
{
    EXPECT_EQ(landmark_->getFaceLandmarkAt(0), cv::Point());
}

TEST_F(
    TestFaceLandmark, ReturnsZeroPoint3DWhenNoFaceRoiExists
)
{
    EXPECT_EQ(landmark_->getFaceLandmark3DAt(0), cv::Point3f());
}

TEST_F(
    TestFaceLandmark, ReturnsEmptyVectorWhenNoFaceRoiExists
)
{
    EXPECT_TRUE(landmark_->getAllFaceLandmarks().empty());
}

TEST_F(
    TestFaceLandmark, Returns468LandmarksWhenFaceIsDetected
)
{
    if (!createImageWithFace())
    {
        GTEST_SKIP() << "No face detected by the test model.";
    }

    auto const landmarks = landmark_->getAllFaceLandmarks();

    EXPECT_EQ(landmarks.size(), 468);
}

TEST_F(
    TestFaceLandmark, ReturnsLandmarkAtFirstIndexWhenFaceIsDetected
)
{
    if (!createImageWithFace())
    {
        GTEST_SKIP() << "No face detected by the test model.";
    }

    auto const point = landmark_->getFaceLandmarkAt(0);

    EXPECT_GE(point.x, 0);
    EXPECT_GE(point.y, 0);
}

TEST_F(
    TestFaceLandmark, ReturnsLandmarkAtLastIndexWhenFaceIsDetected
)
{
    if (!createImageWithFace())
    {
        GTEST_SKIP() << "No face detected by the test model.";
    }

    auto const point = landmark_->getFaceLandmarkAt(467);

    EXPECT_GE(point.x, 0);
    EXPECT_GE(point.y, 0);
}

TEST_F(
    TestFaceLandmark, ReturnsLandmarkModelOutput
)
{
    auto const output = landmark_->loadOutput();

    EXPECT_FALSE(output.empty());
}

TEST_F(
    TestFaceLandmark, ReturnsSameOutputForDifferentIndices
)
{
    auto const output0 = landmark_->loadOutput(0);
    auto const output1 = landmark_->loadOutput(1);

    EXPECT_EQ(output0, output1);
}
