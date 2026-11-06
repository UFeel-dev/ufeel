#include "Emotions/Inference/FaceDetector.hpp"
#include "Library/Emotions/EmotionTestPaths.hpp"
#include "Library/Emotions/Inference/StubFaceDetectorYN.hpp"
#include "opencv2/core/cvstd_wrapper.hpp"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"
#include "opencv2/objdetect/face.hpp"

#include <gtest/gtest.h>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{

auto createFaceRow(
    float x,
    float y,
    float width,
    float height,
    float firstEyeX,
    float firstEyeY,
    float secondEyeX,
    float secondEyeY
) -> cv::Mat
{
    cv::Mat face = cv::Mat::zeros(1, 15, CV_32F);
    auto* values = face.ptr<float>();
    values[0] = x;
    values[1] = y;
    values[2] = width;
    values[3] = height;
    values[4] = firstEyeX;
    values[5] = firstEyeY;
    values[6] = secondEyeX;
    values[7] = secondEyeY;
    return face;
}

auto detectorWithFaces(
    const cv::Mat& faces
) -> std::pair<FaceDetector, cv::Ptr<StubFaceDetectorYN>>
{
    auto stub = cv::makePtr<StubFaceDetectorYN>(faces);
    return {FaceDetector::fromDetector(stub), stub};
}

} // namespace

TEST(
    FaceDetectorTest, ThrowsWhenModelDoesNotExist
)
{
    EXPECT_ANY_THROW(const FaceDetector detector(std::string("missing-face-detector.onnx")));
}

TEST(
    FaceDetectorTest, RejectsNullInjectedDetector
)
{
    EXPECT_THROW(
        FaceDetector::fromDetector(cv::Ptr<cv::FaceDetectorYN>{}), std::runtime_error
    ); // NOLINT
}

TEST(
    FaceDetectorTest, ReturnsNoDetectionWhenModelFindsNoFaces
)
{
    auto [detector, stub] = detectorWithFaces({});

    EXPECT_FALSE(detector.detect(cv::Mat(40, 50, CV_8UC3)).has_value());
    EXPECT_EQ(stub->getInputSize(), cv::Size(50, 40));
}

TEST(
    FaceDetectorTest, ReturnsLargestFaceAndSortsEyeCoordinates
)
{
    const cv::Mat faces = [] -> cv::Mat {
        cv::Mat rows(2, 15, CV_32F, cv::Scalar(0));
        createFaceRow(1.0F, 2.0F, 20.0F, 20.0F, 5.0F, 10.0F, 15.0F, 10.0F).copyTo(rows.row(0));
        createFaceRow(3.0F, 4.0F, 30.0F, 30.0F, 25.0F, 12.0F, 10.0F, 12.0F).copyTo(rows.row(1));
        return rows;
    }();
    auto [detector, stub] = detectorWithFaces(faces);

    const auto detection = detector.detect(cv::Mat(40, 50, CV_8UC3));

    if (!detection)
    {
        FAIL() << "Expected the face detector to return a detection.";
        return;
    }
    const auto& face = *detection;
    EXPECT_EQ(face.box, cv::Rect2f(3.0F, 4.0F, 30.0F, 30.0F));
    EXPECT_EQ(face.leftEye, cv::Point2f(10.0F, 12.0F));
    EXPECT_EQ(face.rightEye, cv::Point2f(25.0F, 12.0F));
}

TEST(
    FaceDetectorTest, KeepsEyeCoordinatesWhenAlreadyOrdered
)
{
    auto [detector, stub] =
        detectorWithFaces(createFaceRow(0.0F, 0.0F, 20.0F, 20.0F, 4.0F, 8.0F, 14.0F, 8.0F));

    const auto detection = detector.detect(cv::Mat(40, 50, CV_8UC3));

    if (!detection)
    {
        FAIL() << "Expected the face detector to return a detection.";
        return;
    }
    const auto& face = *detection;
    EXPECT_EQ(face.leftEye, cv::Point2f(4.0F, 8.0F));
    EXPECT_EQ(face.rightEye, cv::Point2f(14.0F, 8.0F));
}

TEST(
    FaceDetectorTest, LoadsModelAndReturnsNoDetectionForBlankImage
)
{
    FaceDetector detector(emotionFaceDetectorModelPath());
    const cv::Mat image(240, 320, CV_8UC3, cv::Scalar::all(0));

    EXPECT_FALSE(detector.detect(image).has_value());
}
