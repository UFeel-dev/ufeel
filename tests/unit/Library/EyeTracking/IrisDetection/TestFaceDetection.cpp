#include "EyeTracking/IrisDetection/DetectionPostProcess.hpp"
#include "EyeTracking/IrisDetection/FaceDetection.hpp"
#include "opencv2/core.hpp"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/matx.hpp"
#include "opencv2/core/types.hpp"

#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#ifndef MODEL_DIR_PATH
#define MODEL_DIR_PATH ""
#endif

namespace
{

class TestFaceDetection : public ::testing::Test
{
protected:

    void SetUp() override
    {
        detector_ = std::make_unique<my::FaceDetection>(MODEL_DIR_PATH);
    }

    [[nodiscard]] static auto createImage(
        int width = 640, int height = 480, int type = CV_8UC3
    ) -> cv::Mat
    {
        return {height, width, type, cv::Scalar(100, 150, 200, 255)};
    }

    std::unique_ptr<my::FaceDetection> detector_;
};

class TestableFaceDetection final : public my::FaceDetection
{
public:

    explicit TestableFaceDetection(
        my::Detection detection
    )
        : my::FaceDetection(MODEL_DIR_PATH)
        , detection_(detection)
    {
    }

protected:

    [[nodiscard]] auto detectFace(
        const std::vector<float>& regressor, const std::vector<float>& classificator
    ) const -> my::Detection override
    {
        static_cast<void>(regressor);
        static_cast<void>(classificator);
        return detection_;
    }

private:

    my::Detection detection_;
};

} // namespace

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

TEST_F(
    TestFaceDetection, ReturnsFaceRegressor
)
{
    auto const regressor = detector_->getFaceRegressor();

    EXPECT_FALSE(regressor.empty());
}

TEST_F(
    TestFaceDetection, ReturnsFaceClassificator
)
{
    auto const classificator = detector_->getFaceClassificator();

    EXPECT_FALSE(classificator.empty());
}

TEST_F(
    TestFaceDetection, ReturnsEmptyRoiBeforeInference
)
{
    auto const roi = detector_->getFaceRoi();

    EXPECT_TRUE(roi.empty());
}

TEST_F(
    TestFaceDetection, ReturnsEmptyRoiAfterLoadingImage
)
{
    auto const image = createImage();

    detector_->loadImageToInput(image, 0);

    auto const roi = detector_->getFaceRoi();

    EXPECT_TRUE(roi.empty());
}

TEST_F(
    TestFaceDetection, RunsInferenceAfterLoadingImage
)
{
    auto const image = createImage();

    detector_->loadImageToInput(image, 0);

    EXPECT_NO_THROW(detector_->runInference());
}

TEST_F(
    TestFaceDetection, ReturnsOriginalImageAfterInference
)
{
    auto const image = createImage();

    detector_->loadImageToInput(image, 0);
    detector_->runInference();

    auto const original = detector_->getOriginalImage();

    EXPECT_FALSE(original.empty());
    EXPECT_EQ(original.size(), image.size());
}

TEST_F(
    TestFaceDetection, ReturnsFaceRoiAfterInference
)
{
    auto const image = createImage();

    detector_->loadImageToInput(image, 0);
    detector_->runInference();

    auto const roi = detector_->getFaceRoi();

    EXPECT_GE(roi.width, 0);
    EXPECT_GE(roi.height, 0);
}

TEST_F(
    TestFaceDetection, ReturnsEmptyImageWhenOriginalImageIsEmpty
)
{
    auto const cropped = detector_->cropFrame(cv::Rect(10, 10, 100, 100));

    EXPECT_TRUE(cropped.empty());
}

TEST_F(
    TestFaceDetection, ReturnsEmptyImageForZeroWidthRoi
)
{
    auto const image = createImage();

    detector_->loadImageToInput(image, 0);

    auto const cropped = detector_->cropFrame(cv::Rect(10, 10, 0, 100));

    EXPECT_TRUE(cropped.empty());
}

TEST_F(
    TestFaceDetection, ReturnsEmptyImageForZeroHeightRoi
)
{
    auto const image = createImage();

    detector_->loadImageToInput(image, 0);

    auto const cropped = detector_->cropFrame(cv::Rect(10, 10, 100, 0));

    EXPECT_TRUE(cropped.empty());
}

TEST_F(
    TestFaceDetection, CropsImageInsideFrame
)
{
    auto const image = createImage();

    detector_->loadImageToInput(image, 0);

    cv::Rect const roi(100, 100, 200, 150);

    auto const cropped = detector_->cropFrame(roi);

    EXPECT_EQ(cropped.size(), roi.size());
    EXPECT_EQ(cropped.type(), image.type());
    EXPECT_FALSE(cropped.empty());
}

TEST_F(
    TestFaceDetection, CropsImageWithRoiOutsideLeftBoundary
)
{
    auto const image = createImage();

    detector_->loadImageToInput(image, 0);

    cv::Rect const roi(-50, 100, 200, 150);

    auto const cropped = detector_->cropFrame(roi);

    ASSERT_EQ(cropped.size(), roi.size());
    EXPECT_EQ(cropped.at<cv::Vec3b>(0, 0), cv::Vec3b(0, 0, 0));
    EXPECT_EQ(cropped.at<cv::Vec3b>(0, 50), image.at<cv::Vec3b>(100, 0));
}

TEST_F(
    TestFaceDetection, CropsImageWithRoiOutsideRightBoundary
)
{
    auto const image = createImage();

    detector_->loadImageToInput(image, 0);

    cv::Rect const roi(600, 100, 100, 150);

    auto const cropped = detector_->cropFrame(roi);

    ASSERT_EQ(cropped.size(), roi.size());
    EXPECT_EQ(cropped.at<cv::Vec3b>(0, 0), image.at<cv::Vec3b>(100, 600));
    EXPECT_EQ(cropped.at<cv::Vec3b>(0, 50), cv::Vec3b(0, 0, 0));
}

TEST_F(
    TestFaceDetection, CropsImageWithRoiOutsideTopBoundary
)
{
    auto const image = createImage();

    detector_->loadImageToInput(image, 0);

    cv::Rect const roi(100, -50, 200, 150);

    auto const cropped = detector_->cropFrame(roi);

    ASSERT_EQ(cropped.size(), roi.size());
    EXPECT_EQ(cropped.at<cv::Vec3b>(0, 0), cv::Vec3b(0, 0, 0));
    EXPECT_EQ(cropped.at<cv::Vec3b>(50, 0), image.at<cv::Vec3b>(0, 100));
}

TEST_F(
    TestFaceDetection, CropsImageWithRoiOutsideBottomBoundary
)
{
    auto const image = createImage();

    detector_->loadImageToInput(image, 0);

    cv::Rect const roi(100, 400, 200, 150);

    auto const cropped = detector_->cropFrame(roi);

    ASSERT_EQ(cropped.size(), roi.size());
    EXPECT_EQ(cropped.at<cv::Vec3b>(0, 0), image.at<cv::Vec3b>(400, 100));
    EXPECT_EQ(cropped.at<cv::Vec3b>(100, 0), cv::Vec3b(0, 0, 0));
}

TEST_F(
    TestFaceDetection, ReturnsZeroFilledImageWhenRoiDoesNotIntersectFrame
)
{
    auto const image = createImage();
    detector_->loadImageToInput(image, 0);

    auto const cropped = detector_->cropFrame(cv::Rect(700, 500, 100, 100));

    ASSERT_EQ(cropped.size(), cv::Size(100, 100));
    EXPECT_EQ(cv::countNonZero(cropped.reshape(1)), 0);
}

TEST(
    FaceDetectionInjectionTest, RejectsNonFiniteDetectionCoordinates
)
{
    TestableFaceDetection detector(
        my::Detection(0.9F, 0, cv::Rect2f(std::numeric_limits<float>::quiet_NaN(), 0, 1, 1))
    );
    detector.loadImageToInput(cv::Mat(480, 640, CV_8UC3, cv::Scalar::all(128)), 0);

    ASSERT_NO_THROW(detector.runInference());
    EXPECT_TRUE(detector.getFaceRoi().empty());
}

TEST(
    FaceDetectionInjectionTest, RejectsDetectionWithInsufficientDimensions
)
{
    TestableFaceDetection detector(my::Detection(0.9F, 0, cv::Rect2f(0, 0, 0, 1)));
    detector.loadImageToInput(cv::Mat(480, 640, CV_8UC3, cv::Scalar::all(128)), 0);

    ASSERT_NO_THROW(detector.runInference());
    EXPECT_TRUE(detector.getFaceRoi().empty());
}
