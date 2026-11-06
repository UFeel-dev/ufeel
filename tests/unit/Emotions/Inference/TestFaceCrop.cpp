#include "Emotions/Inference/FaceCrop.hpp"
#include "Emotions/Inference/FaceDetector.hpp"
#include "StubFaceDetectorYN.hpp"
#include "opencv2/core/cvstd_wrapper.hpp"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/matx.hpp"
#include "opencv2/core/types.hpp"

#include <gtest/gtest.h>

namespace
{

auto faceCropWithDetection(
    const cv::Rect2f& box,
    const cv::Point2f& leftEye,
    const cv::Point2f& rightEye,
    int size = 64,
    float margin = 0.0F
) -> FaceCrop
{
    cv::Mat faces = cv::Mat::zeros(1, 15, CV_32F);
    auto* values = faces.ptr<float>();
    values[0] = box.x;
    values[1] = box.y;
    values[2] = box.width;
    values[3] = box.height;
    values[4] = leftEye.x;
    values[5] = leftEye.y;
    values[6] = rightEye.x;
    values[7] = rightEye.y;

    return {FaceDetector::fromDetector(cv::makePtr<StubFaceDetectorYN>(faces)), size, margin};
}

} // namespace

TEST(
    FaceCropTest, ResizesFrameToModelInputWhenNoFaceIsDetected
)
{
    FaceCrop crop(FaceDetector::fromDetector(cv::makePtr<StubFaceDetectorYN>(cv::Mat{})), 64);
    const cv::Mat image(80, 120, CV_8UC3, cv::Scalar(10, 20, 30));

    const auto cropped = crop(image);

    ASSERT_EQ(cropped.size(), cv::Size(64, 64));
    EXPECT_EQ(cropped.type(), CV_8UC3);
    EXPECT_EQ(cropped.at<cv::Vec3b>(32, 32), cv::Vec3b(10, 20, 30));
}

TEST(
    FaceCropTest, PreservesSquareInputAtModelSizeWhenNoFaceIsDetected
)
{
    FaceCrop crop(FaceDetector::fromDetector(cv::makePtr<StubFaceDetectorYN>(cv::Mat{})), 64);
    const cv::Mat image(64, 64, CV_8UC3, cv::Scalar(40, 50, 60));

    const auto cropped = crop(image);

    ASSERT_EQ(cropped.size(), image.size());
    EXPECT_EQ(cropped.at<cv::Vec3b>(32, 32), image.at<cv::Vec3b>(32, 32));
}

TEST(
    FaceCropTest, ResizesTallFrameUsingAreaInterpolationWhenNoFaceIsDetected
)
{
    FaceCrop crop(FaceDetector::fromDetector(cv::makePtr<StubFaceDetectorYN>(cv::Mat{})), 64);
    const cv::Mat image(120, 40, CV_8UC3, cv::Scalar(40, 50, 60));

    const auto cropped = crop(image);

    ASSERT_EQ(cropped.size(), cv::Size(64, 64));
    EXPECT_EQ(cropped.at<cv::Vec3b>(32, 32), cv::Vec3b(40, 50, 60));
}

TEST(
    FaceCropTest, CropsAndPadsDetectedFaceToSquareModelInput
)
{
    FaceCrop crop = faceCropWithDetection(
        cv::Rect2f(20.0F, 30.0F, 40.0F, 20.0F), cv::Point2f(25.0F, 40.0F), cv::Point2f(55.0F, 40.0F)
    );
    const cv::Mat image(100, 100, CV_8UC3, cv::Scalar(10, 20, 30));

    const auto cropped = crop(image);

    ASSERT_EQ(cropped.size(), cv::Size(64, 64));
    EXPECT_EQ(cropped.at<cv::Vec3b>(0, 0), cv::Vec3b(0, 0, 0));
    EXPECT_EQ(cropped.at<cv::Vec3b>(32, 32), cv::Vec3b(10, 20, 30));
}

TEST(
    FaceCropTest, ClipsFaceBoundsToImage
)
{
    FaceCrop crop = faceCropWithDetection(
        cv::Rect2f(-10.0F, -10.0F, 40.0F, 40.0F),
        cv::Point2f(0.0F, 0.0F),
        cv::Point2f(20.0F, 0.0F),
        64,
        0.1F
    );
    const cv::Mat image(50, 50, CV_8UC3, cv::Scalar(10, 20, 30));

    const auto cropped = crop(image);

    EXPECT_EQ(cropped.size(), cv::Size(64, 64));
    EXPECT_NE(cropped.at<cv::Vec3b>(32, 32), cv::Vec3b(0, 0, 0));
}

TEST(
    FaceCropTest, FallsBackToResizedFrameWhenFaceBoxDoesNotIntersectImage
)
{
    FaceCrop crop = faceCropWithDetection(
        cv::Rect2f(120.0F, 120.0F, 10.0F, 10.0F),
        cv::Point2f(122.0F, 125.0F),
        cv::Point2f(128.0F, 125.0F)
    );
    const cv::Mat image(100, 100, CV_8UC3, cv::Scalar(70, 80, 90));

    const auto cropped = crop(image);

    ASSERT_EQ(cropped.size(), cv::Size(64, 64));
    EXPECT_EQ(cropped.at<cv::Vec3b>(32, 32), cv::Vec3b(70, 80, 90));
}

TEST(
    FaceCropTest, FallsBackWhenFaceBoxHasNoHeight
)
{
    FaceCrop crop = faceCropWithDetection(
        cv::Rect2f(20.0F, 30.0F, 10.0F, 0.0F), cv::Point2f(22.0F, 30.0F), cv::Point2f(28.0F, 30.0F)
    );
    const cv::Mat image(100, 100, CV_8UC3, cv::Scalar(70, 80, 90));

    const auto cropped = crop(image);

    ASSERT_EQ(cropped.size(), cv::Size(64, 64));
    EXPECT_EQ(cropped.at<cv::Vec3b>(32, 32), cv::Vec3b(70, 80, 90));
}

TEST(
    FaceCropTest, AlignsDetectedFaceUsingEyeAngle
)
{
    FaceCrop crop = faceCropWithDetection(
        cv::Rect2f(10.0F, 10.0F, 60.0F, 40.0F), cv::Point2f(20.0F, 30.0F), cv::Point2f(50.0F, 40.0F)
    );
    const cv::Mat image(80, 80, CV_8UC3, cv::Scalar(90, 100, 110));

    const auto cropped = crop(image);

    ASSERT_EQ(cropped.size(), cv::Size(64, 64));
    EXPECT_EQ(cropped.at<cv::Vec3b>(32, 32), cv::Vec3b(90, 100, 110));
}
