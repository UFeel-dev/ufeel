#include "EyeTracking/IrisDetection/IrisLandmark.hpp"

#include <gtest/gtest.h>
#include <memory>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <string>
#include <vector>

class TestIrisLandmark : public ::testing::Test
{
protected:

    void SetUp() override
    {
        irisLandmark_ = std::make_unique<my::IrisLandmark>(MODEL_DIR_PATH);
    }

    auto prepareInference() -> bool
    {
        cv::Mat image =
            cv::imread(std::string(RESOURCES_DIR_PATH) + "/astronaut.png", cv::IMREAD_COLOR);

        if (image.empty())
        {
            return false;
        }

        irisLandmark_->loadImageToInput(image);
        irisLandmark_->runInference();

        return !irisLandmark_->getFaceRoi().empty();
    }

    std::unique_ptr<my::IrisLandmark> irisLandmark_;
};

TEST_F(
    TestIrisLandmark, ConstructorSucceedsWithValidModels
)
{
    EXPECT_NE(irisLandmark_, nullptr);
}

TEST_F(
    TestIrisLandmark, EyeRoisAreEmptyBeforeInference
)
{
    EXPECT_TRUE(irisLandmark_->getEyeRoi(true).empty());
    EXPECT_TRUE(irisLandmark_->getEyeRoi(false).empty());
}

TEST_F(
    TestIrisLandmark, InitialState
)
{
    EXPECT_TRUE(irisLandmark_->getEyeRoi(true).empty());
    EXPECT_TRUE(irisLandmark_->getEyeRoi(false).empty());

    EXPECT_TRUE(irisLandmark_->getAllEyeLandmarks(true, false).empty());
    EXPECT_TRUE(irisLandmark_->getAllEyeLandmarks(false, false).empty());

    EXPECT_TRUE(irisLandmark_->getAllEyeLandmarks(true, true).empty());
    EXPECT_TRUE(irisLandmark_->getAllEyeLandmarks(false, true).empty());
}

TEST_F(
    TestIrisLandmark, LoadImageAndRunInference
)
{
    if (!prepareInference())
    {
        GTEST_SKIP() << "No face detected in astronaut.png.";
    }

    EXPECT_FALSE(irisLandmark_->getFaceRoi().empty());
    EXPECT_FALSE(irisLandmark_->getEyeRoi(true).empty());
    EXPECT_FALSE(irisLandmark_->getEyeRoi(false).empty());
}

TEST_F(
    TestIrisLandmark, EyeRoisAreValidAndSquare
)
{
    if (!prepareInference())
    {
        GTEST_SKIP() << "No face detected in astronaut.png.";
    }

    const cv::Rect leftRoi = irisLandmark_->getEyeRoi(true);
    const cv::Rect rightRoi = irisLandmark_->getEyeRoi(false);

    ASSERT_FALSE(leftRoi.empty());
    ASSERT_FALSE(rightRoi.empty());

    EXPECT_GT(leftRoi.width, 0);
    EXPECT_GT(leftRoi.height, 0);
    EXPECT_EQ(leftRoi.width, leftRoi.height);

    EXPECT_GT(rightRoi.width, 0);
    EXPECT_GT(rightRoi.height, 0);
    EXPECT_EQ(rightRoi.width, rightRoi.height);
}

TEST_F(
    TestIrisLandmark, GetFirstAndLastEyeLandmarks
)
{
    if (!prepareInference())
    {
        GTEST_SKIP() << "No face detected in astronaut.png.";
    }

    const auto leftLandmarks = irisLandmark_->getAllEyeLandmarks(true, false);

    const auto rightLandmarks = irisLandmark_->getAllEyeLandmarks(false, false);

    ASSERT_EQ(leftLandmarks.size(), 71);
    ASSERT_EQ(rightLandmarks.size(), 71);

    EXPECT_EQ(irisLandmark_->getEyeLandmarkAt(0, true, false), leftLandmarks.front());
    EXPECT_EQ(irisLandmark_->getEyeLandmarkAt(70, true, false), leftLandmarks.back());
    EXPECT_EQ(irisLandmark_->getEyeLandmarkAt(0, false, false), rightLandmarks.front());
    EXPECT_EQ(irisLandmark_->getEyeLandmarkAt(70, false, false), rightLandmarks.back());
}

TEST_F(
    TestIrisLandmark, InvalidEyeLandmarkIndicesReturnZero
)
{
    if (!prepareInference())
    {
        GTEST_SKIP() << "No face detected in astronaut.png.";
    }

    EXPECT_EQ(irisLandmark_->getEyeLandmarkAt(-1, true, false), cv::Point());
    EXPECT_EQ(irisLandmark_->getEyeLandmarkAt(71, true, false), cv::Point());
    EXPECT_EQ(irisLandmark_->getEyeLandmarkAt(-1, false, false), cv::Point());
    EXPECT_EQ(irisLandmark_->getEyeLandmarkAt(71, false, false), cv::Point());
}

TEST_F(
    TestIrisLandmark, GetAllLandmarksReturnsExpectedCounts
)
{
    if (!prepareInference())
    {
        GTEST_SKIP() << "No face detected in astronaut.png.";
    }

    EXPECT_EQ(irisLandmark_->getAllEyeLandmarks(true, false).size(), 71);
    EXPECT_EQ(irisLandmark_->getAllEyeLandmarks(false, false).size(), 71);
    EXPECT_EQ(irisLandmark_->getAllEyeLandmarks(true, true).size(), 5);
    EXPECT_EQ(irisLandmark_->getAllEyeLandmarks(false, true).size(), 5);
}

TEST_F(
    TestIrisLandmark, GetFirstAndLastIrisLandmarks
)
{
    if (!prepareInference())
    {
        GTEST_SKIP() << "No face detected in astronaut.png.";
    }

    const auto leftLandmarks = irisLandmark_->getAllEyeLandmarks(true, true);

    const auto rightLandmarks = irisLandmark_->getAllEyeLandmarks(false, true);

    ASSERT_EQ(leftLandmarks.size(), 5);
    ASSERT_EQ(rightLandmarks.size(), 5);

    EXPECT_EQ(irisLandmark_->getEyeLandmarkAt(0, true, true), leftLandmarks.front());
    EXPECT_EQ(irisLandmark_->getEyeLandmarkAt(4, true, true), leftLandmarks.back());
    EXPECT_EQ(irisLandmark_->getEyeLandmarkAt(0, false, true), rightLandmarks.front());
    EXPECT_EQ(irisLandmark_->getEyeLandmarkAt(4, false, true), rightLandmarks.back());
}

TEST_F(
    TestIrisLandmark, InvalidIrisLandmarkIndicesReturnZero
)
{
    if (!prepareInference())
    {
        GTEST_SKIP() << "No face detected in astronaut.png.";
    }

    EXPECT_EQ(irisLandmark_->getEyeLandmarkAt(-1, true, true), cv::Point());
    EXPECT_EQ(irisLandmark_->getEyeLandmarkAt(5, true, true), cv::Point());
    EXPECT_EQ(irisLandmark_->getEyeLandmarkAt(-1, false, true), cv::Point());
    EXPECT_EQ(irisLandmark_->getEyeLandmarkAt(5, false, true), cv::Point());
}

TEST_F(
    TestIrisLandmark, LoadOutputsForBothEyesAndLandmarkTypes
)
{
    if (!prepareInference())
    {
        GTEST_SKIP() << "No face detected in astronaut.png.";
    }

    EXPECT_FALSE(irisLandmark_->loadOutput(0, true).empty());
    EXPECT_FALSE(irisLandmark_->loadOutput(0, false).empty());
    EXPECT_FALSE(irisLandmark_->loadOutput(1, true).empty());
    EXPECT_FALSE(irisLandmark_->loadOutput(1, false).empty());
}

TEST_F(
    TestIrisLandmark, DefaultLoadOutputReturnsLeftEyeOutput
)
{
    if (!prepareInference())
    {
        GTEST_SKIP() << "No face detected in astronaut.png.";
    }

    const auto defaultOutput = irisLandmark_->loadOutput();
    const auto leftEyeOutput = irisLandmark_->loadOutput(0, true);

    ASSERT_FALSE(defaultOutput.empty());
    ASSERT_EQ(defaultOutput.size(), leftEyeOutput.size());

    EXPECT_EQ(defaultOutput, leftEyeOutput);
}
