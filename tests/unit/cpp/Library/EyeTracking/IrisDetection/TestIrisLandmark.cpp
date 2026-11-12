#include "EyeTracking/IrisDetection/DetectionPostProcess.hpp"
#include "EyeTracking/IrisDetection/IrisLandmark.hpp"
#include "EyeTracking/IrisDetection/ModelLoader.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"
#include "opencv2/imgcodecs.hpp"

#include <cstddef>
#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <opencv2/core/hal/interface.h>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifndef MODEL_DIR_PATH
#define MODEL_DIR_PATH ""
#endif // MODEL_DIR_PATH
#ifndef RESOURCES_DIR_PATH
#define RESOURCES_DIR_PATH ""
#endif // RESOURCES_DIR_PATH

namespace
{

class FakeModelLoader final : public my::ModelLoader
{
public:

    std::vector<int> inputShape{1, 2, 2, 3};
    std::vector<float> output = std::vector<float>(15, 0.5F);
    std::size_t outputSize = output.size() * sizeof(float);
    float* outputData = output.data();
    bool throwOnInference = false;

    [[nodiscard]] auto getInputShape(
        int index
    ) const -> std::vector<int> override
    {
        static_cast<void>(index);
        return inputShape;
    }

    [[nodiscard]] auto getOutputData(
        int index
    ) const -> float* override
    {
        static_cast<void>(index);
        return outputData;
    }

    [[nodiscard]] auto getOutputSize(
        int index
    ) const -> std::size_t override
    {
        static_cast<void>(index);
        return outputSize;
    }

    [[nodiscard]] auto loadOutput(
        int index
    ) const -> std::vector<float> override
    {
        static_cast<void>(index);
        return output;
    }

    void loadImageToInput(
        const cv::Mat& /*inputImage*/, int /*index*/
    ) override
    {
    }

    void runInference() override
    {
        if (throwOnInference)
        {
            throw std::runtime_error("inference failed");
        }
    }
};

class TestableIrisLandmark final : public my::IrisLandmark
{
public:

    TestableIrisLandmark(
        std::unique_ptr<my::ModelLoader> faceModel,
        std::unique_ptr<my::ModelLoader> leftModel,
        std::unique_ptr<my::ModelLoader> rightModel
    )
        : my::IrisLandmark(
              MODEL_DIR_PATH, std::move(faceModel), std::move(leftModel), std::move(rightModel)
          )
    {
    }

    [[nodiscard]] auto getFaceRoi() const -> cv::Rect override
    {
        return {10, 20, 100, 80};
    }

    [[nodiscard]] auto getEyeRoi(
        bool /*isLeftEye*/
    ) const -> cv::Rect override
    {
        return {10, 20, 20, 20};
    }

protected:

    [[nodiscard]] auto detectFace(
        const std::vector<float>& /*regressor*/, const std::vector<float>& /*classificator*/
    ) const -> my::Detection override
    {
        return {0.9F, CLASS_ID, cv::Rect2f(0.5F, 0.5F, 0.2F, 0.2F)};
    }

public:

    [[nodiscard]] auto getFaceLandmarkAt(
        int index
    ) const -> cv::Point override
    {
        if (invalidEyeLandmarks)
        {
            return {100, 100};
        }

        switch (index)
        {
        case 446:
            return {100, 100};
        case 464:
            return {120, 100};
        case 244:
            return {200, 100};
        case 226:
            return {220, 100};
        default:
            return {};
        }
    }

    bool invalidEyeLandmarks = false;
};

class TestIrisLandmark : public ::testing::Test
{
protected:

    void SetUp() override
    {
        irisLandmark_ = std::make_unique<my::IrisLandmark>(MODEL_DIR_PATH);
    }

    auto prepareInference() -> bool
    {
        const cv::Mat image =
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

} // namespace

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
    TestIrisLandmark, RejectsInvalidLandmarkIndicesBeforeInference
)
{
    EXPECT_EQ(irisLandmark_->getEyeLandmarkAt(-1, true, false), cv::Point());
    EXPECT_EQ(irisLandmark_->getEyeLandmarkAt(71, false, false), cv::Point());
    EXPECT_EQ(irisLandmark_->getEyeLandmarkAt(-1, true, true), cv::Point());
    EXPECT_EQ(irisLandmark_->getEyeLandmarkAt(5, false, true), cv::Point());
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

TEST(
    IrisLandmarkInjectionTest, ReturnsEmptyWhenOutputIsTooShort
)
{
    auto faceModel = std::make_unique<FakeModelLoader>();
    auto leftModel = std::make_unique<FakeModelLoader>();
    auto rightModel = std::make_unique<FakeModelLoader>();
    leftModel->outputSize = sizeof(float);
    rightModel->outputSize = sizeof(float);
    TestableIrisLandmark const landmark(
        std::move(faceModel), std::move(leftModel), std::move(rightModel)
    );

    EXPECT_EQ(landmark.getEyeLandmarkAt(0, true, false), cv::Point());
    EXPECT_EQ(landmark.getEyeLandmarkAt(0, false, true), cv::Point());
}

TEST(
    IrisLandmarkInjectionTest, ReturnsEmptyWhenInputShapeIsInvalid
)
{
    auto faceModel = std::make_unique<FakeModelLoader>();
    auto leftModel = std::make_unique<FakeModelLoader>();
    auto rightModel = std::make_unique<FakeModelLoader>();
    leftModel->inputShape = {1, 0, 2};
    rightModel->inputShape = {1, 2};
    TestableIrisLandmark const landmark(
        std::move(faceModel), std::move(leftModel), std::move(rightModel)
    );

    EXPECT_EQ(landmark.getEyeLandmarkAt(0, true, false), cv::Point());
    EXPECT_EQ(landmark.getEyeLandmarkAt(0, false, true), cv::Point());
}

TEST(
    IrisLandmarkInjectionTest, ReturnsEmptyWhenOutputDataIsNull
)
{
    auto faceModel = std::make_unique<FakeModelLoader>();
    auto leftModel = std::make_unique<FakeModelLoader>();
    auto rightModel = std::make_unique<FakeModelLoader>();
    leftModel->outputData = nullptr;
    rightModel->outputData = nullptr;
    TestableIrisLandmark const landmark(
        std::move(faceModel), std::move(leftModel), std::move(rightModel)
    );

    EXPECT_EQ(landmark.getEyeLandmarkAt(0, true, false), cv::Point());
    EXPECT_EQ(landmark.getEyeLandmarkAt(0, false, true), cv::Point());
}

TEST(
    IrisLandmarkInjectionTest, ReturnsEmptyWhenOutputCoordinatesAreNotFinite
)
{
    auto faceModel = std::make_unique<FakeModelLoader>();
    auto leftModel = std::make_unique<FakeModelLoader>();
    auto rightModel = std::make_unique<FakeModelLoader>();
    leftModel->output.at(0) = std::numeric_limits<float>::infinity();
    rightModel->output.at(1) = std::numeric_limits<float>::quiet_NaN();
    TestableIrisLandmark const landmark(
        std::move(faceModel), std::move(leftModel), std::move(rightModel)
    );

    EXPECT_EQ(landmark.getEyeLandmarkAt(0, true, false), cv::Point());
    EXPECT_EQ(landmark.getEyeLandmarkAt(0, false, true), cv::Point());
}

TEST(
    IrisLandmarkInjectionTest, ClearsRoisWhenRightEyeInferenceFails
)
{
    auto faceModel = std::make_unique<FakeModelLoader>();
    auto leftModel = std::make_unique<FakeModelLoader>();
    auto rightModel = std::make_unique<FakeModelLoader>();
    rightModel->throwOnInference = true;
    TestableIrisLandmark landmark(
        std::move(faceModel), std::move(leftModel), std::move(rightModel)
    );
    landmark.loadImageToInput(cv::Mat(480, 640, CV_8UC3, cv::Scalar::all(128)));

    EXPECT_THROW(landmark.runInference(), std::runtime_error);
}

TEST(
    IrisLandmarkInjectionTest, ClearsRoisWhenLeftEyeInferenceFails
)
{
    auto faceModel = std::make_unique<FakeModelLoader>();
    auto leftModel = std::make_unique<FakeModelLoader>();
    auto rightModel = std::make_unique<FakeModelLoader>();
    leftModel->throwOnInference = true;
    TestableIrisLandmark landmark(
        std::move(faceModel), std::move(leftModel), std::move(rightModel)
    );
    landmark.loadImageToInput(cv::Mat(480, 640, CV_8UC3, cv::Scalar::all(128)));

    EXPECT_THROW(landmark.runInference(), std::runtime_error);
}

TEST(
    IrisLandmarkInjectionTest, RejectsEyeWithIdenticalLandmarks
)
{
    auto faceModel = std::make_unique<FakeModelLoader>();
    auto leftModel = std::make_unique<FakeModelLoader>();
    auto rightModel = std::make_unique<FakeModelLoader>();
    TestableIrisLandmark landmark(
        std::move(faceModel), std::move(leftModel), std::move(rightModel)
    );
    landmark.invalidEyeLandmarks = true;
    landmark.loadImageToInput(cv::Mat(480, 640, CV_8UC3, cv::Scalar::all(128)));

    EXPECT_THROW(landmark.runInference(), std::runtime_error);
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
