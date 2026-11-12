#include "EyeTracking/IrisDetection/FaceLandmark.hpp"
#include "EyeTracking/IrisDetection/ModelLoader.hpp"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"
#include "opencv2/imgcodecs.hpp"

#include <cmath>
#include <cstddef>
#include <gtest/gtest.h>
#include <limits>
#include <memory>
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
    std::vector<float> output{0.5F, 0.25F, 0.1F};
    std::size_t outputSize = output.size() * sizeof(float);
    float* outputData = output.data();

    [[nodiscard]] auto getInputShape(
        int /*index*/
    ) const -> std::vector<int> override
    {
        return inputShape;
    }

    [[nodiscard]] auto getOutputData(
        int /*index*/
    ) const -> float* override
    {
        return outputData;
    }

    [[nodiscard]] auto getOutputSize(
        int /*index*/
    ) const -> std::size_t override
    {
        return outputSize;
    }

    [[nodiscard]] auto loadOutput(
        int /*index*/
    ) const -> std::vector<float> override
    {
        return output;
    }
};

class TestableFaceLandmark final : public my::FaceLandmark
{
public:

    explicit TestableFaceLandmark(
        std::unique_ptr<my::ModelLoader> model
    )
        : my::FaceLandmark(MODEL_DIR_PATH, std::move(model))
    {
    }

    cv::Rect faceRoi{10, 20, 100, 80};

    [[nodiscard]] auto getFaceRoi() const -> cv::Rect override
    {
        return faceRoi;
    }
};

class TestFaceLandmark : public ::testing::Test
{
protected:

    void SetUp() override
    {
        landmark_ = std::make_unique<my::FaceLandmark>(MODEL_DIR_PATH);
    }

    [[nodiscard]] static auto createImage(
        int width = 640, int height = 480
    ) -> cv::Mat
    {
        return {height, width, CV_8UC3, cv::Scalar(100, 150, 200)};
    }

    auto createImageWithFace() -> bool
    {
        const cv::Mat image =
            cv::imread(std::string(RESOURCES_DIR_PATH) + "/astronaut.png", cv::IMREAD_COLOR);

        if (image.empty())
        {
            return false;
        }

        landmark_->loadImageToInput(image, 0);
        landmark_->runInference();

        return !landmark_->getFaceRoi().empty();
    }

    std::unique_ptr<my::FaceLandmark> landmark_;

public:

    TestFaceLandmark() = default;
    ~TestFaceLandmark() override = default;

    TestFaceLandmark(const TestFaceLandmark&) = delete;
    TestFaceLandmark(TestFaceLandmark&&) = delete;
    auto operator=(const TestFaceLandmark&) -> TestFaceLandmark& = delete;
    auto operator=(TestFaceLandmark&&) -> TestFaceLandmark& = delete;
};

} // namespace

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
    TestFaceLandmark, RejectsInvalidLandmarkIndicesBeforeInference
)
{
    EXPECT_EQ(landmark_->getFaceLandmarkAt(-1), cv::Point());
    EXPECT_EQ(landmark_->getFaceLandmarkAt(468), cv::Point());
    EXPECT_EQ(landmark_->getFaceLandmark3DAt(-1), cv::Point3f());
    EXPECT_EQ(landmark_->getFaceLandmark3DAt(468), cv::Point3f());
}

TEST(
    FaceLandmarkInjectionTest, ReturnsEmptyWhenOutputIsTooShort
)
{
    auto model = std::make_unique<FakeModelLoader>();
    model->outputSize = sizeof(float);
    TestableFaceLandmark const landmark(std::move(model));

    EXPECT_EQ(landmark.getFaceLandmarkAt(0), cv::Point());
    EXPECT_EQ(landmark.getFaceLandmark3DAt(0), cv::Point3f());
}

TEST(
    FaceLandmarkInjectionTest, ReturnsEmptyWhenInputShapeIsInvalid
)
{
    auto model = std::make_unique<FakeModelLoader>();
    model->inputShape = {1, 0, 2};
    TestableFaceLandmark const landmark(std::move(model));

    EXPECT_EQ(landmark.getFaceLandmarkAt(0), cv::Point());
    EXPECT_EQ(landmark.getFaceLandmark3DAt(0), cv::Point3f());
}

TEST(
    FaceLandmarkInjectionTest, ReturnsEmptyWhenOutputDataIsNull
)
{
    auto model = std::make_unique<FakeModelLoader>();
    model->outputData = nullptr;
    TestableFaceLandmark const landmark(std::move(model));

    EXPECT_EQ(landmark.getFaceLandmarkAt(0), cv::Point());
    EXPECT_EQ(landmark.getFaceLandmark3DAt(0), cv::Point3f());
}

TEST(
    FaceLandmarkInjectionTest, ReturnsEmptyWhenOutputCoordinatesAreNotFinite
)
{
    auto model = std::make_unique<FakeModelLoader>();
    model->output.at(0) = std::numeric_limits<float>::quiet_NaN();
    TestableFaceLandmark const landmark(std::move(model));

    EXPECT_EQ(landmark.getFaceLandmarkAt(0), cv::Point());
    EXPECT_EQ(landmark.getFaceLandmark3DAt(0), cv::Point3f());
}

TEST_F(
    TestFaceLandmark, Returns468LandmarksWhenFaceIsDetected
)
{
    ASSERT_TRUE(createImageWithFace()) << "No face detected in astronaut.png.";

    auto const landmarks = landmark_->getAllFaceLandmarks();

    EXPECT_EQ(landmarks.size(), 468);
}

TEST_F(
    TestFaceLandmark, ReturnsLandmarkAtFirstIndexWhenFaceIsDetected
)
{
    ASSERT_TRUE(createImageWithFace()) << "No face detected in astronaut.png.";

    auto const point = landmark_->getFaceLandmarkAt(0);

    EXPECT_GE(point.x, 0);
    EXPECT_GE(point.y, 0);
}

TEST_F(
    TestFaceLandmark, ReturnsLandmarkAtLastIndexWhenFaceIsDetected
)
{
    ASSERT_TRUE(createImageWithFace()) << "No face detected in astronaut.png.";

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
    ASSERT_TRUE(createImageWithFace()) << "No face detected in astronaut.png.";

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
    ASSERT_TRUE(createImageWithFace()) << "No face detected in astronaut.png.";

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
    EXPECT_TRUE(createImageWithFace()) << "No face detected in astronaut.png.";
}
