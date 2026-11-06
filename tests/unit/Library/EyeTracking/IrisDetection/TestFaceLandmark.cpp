#include "EyeTracking/IrisDetection/FaceLandmark.hpp"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"
#include "opencv2/imgcodecs.hpp"

#include <cmath>
#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>
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

#ifndef RESOURCES_DIR_PATH
        FAIL() << "RESOURCES_DIR_PATH is not defined.";
        return;
#else
        resourcesDir_ = RESOURCES_DIR_PATH;
#endif

        landmark_ = std::make_unique<my::FaceLandmark>(modelDir_);
    }

    [[nodiscard]] static auto createImage(
        int width = 640, int height = 480
    ) -> cv::Mat
    {
        return {height, width, CV_8UC3, cv::Scalar(100, 150, 200)};
    }

    auto createImageWithFace() -> bool
    {
        auto const path = resourcesDir_ + "/astronaut.png";
        auto const image = cv::imread(path, cv::IMREAD_COLOR);

        if (image.empty())
        {
            testImageUnavailable_ = true;
            return false;
        }

        landmark_->loadImageToInput(image, 0);
        landmark_->runInference();

        return !landmark_->getFaceRoi().empty();
    }

    std::string modelDir_;
    std::string resourcesDir_;
    std::unique_ptr<my::FaceLandmark> landmark_;
    bool testImageUnavailable_{false};

public:

    TestFaceLandmark() = default;
    ~TestFaceLandmark() override = default;

    TestFaceLandmark(const TestFaceLandmark&) = delete;
    TestFaceLandmark(TestFaceLandmark&&) = delete;
    auto operator=(const TestFaceLandmark&) -> TestFaceLandmark& = delete;
    auto operator=(TestFaceLandmark&&) -> TestFaceLandmark& = delete;
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
        GTEST_SKIP() << "Test image is unavailable or no face was detected.";
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
        GTEST_SKIP() << "Test image is unavailable or no face was detected.";
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
        GTEST_SKIP() << "Test image is unavailable or no face was detected.";
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
    TestFaceLandmark, LandmarkCoordinatesAreInsideOriginalImage
)
{
    if (!createImageWithFace())
    {
        GTEST_SKIP() << "Test image is unavailable or no face was detected.";
    }

    auto const landmarks = landmark_->getAllFaceLandmarks();

    ASSERT_EQ(landmarks.size(), 468);

    auto const image = landmark_->getOriginalImage();

    for (auto const& point : landmarks)
    {
        EXPECT_GE(point.x, 0);
        EXPECT_LT(point.x, image.cols);

        EXPECT_GE(point.y, 0);
        EXPECT_LT(point.y, image.rows);
    }
}

TEST_F(
    TestFaceLandmark, Landmark3DCoordinatesAreFinite
)
{
    if (!createImageWithFace())
    {
        GTEST_SKIP() << "Test image is unavailable or no face was detected.";
    }

    for (int i = 0; i < 468; ++i)
    {
        auto const point = landmark_->getFaceLandmark3DAt(i);

        EXPECT_TRUE(std::isfinite(point.x));
        EXPECT_TRUE(std::isfinite(point.y));
        EXPECT_TRUE(std::isfinite(point.z));
    }
}

TEST_F(
    TestFaceLandmark, DetectsFaceInTestImage
)
{
    const bool detected = createImageWithFace();
    if (testImageUnavailable_)
    {
        GTEST_SKIP() << "Optional test image tests/ressources/face.png is not present.";
    }

    ASSERT_TRUE(detected) << "The test image does not contain a detectable face.";
}
