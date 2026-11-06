#include "../EmotionTestPaths.hpp"
#include "Emotions/Inference/FaceCrop.hpp"
#include "Emotions/Inference/FaceDetector.hpp"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/matx.hpp"
#include "opencv2/core/types.hpp"

#include <gtest/gtest.h>

TEST(
    FaceCropTest, ResizesFrameToModelInputWhenNoFaceIsDetected
)
{
    FaceCrop crop(FaceDetector(emotionFaceDetectorModelPath()), 64);
    const cv::Mat image(80, 120, CV_8UC3, cv::Scalar(10, 20, 30));

    const auto cropped = crop(image);

    ASSERT_EQ(cropped.size(), cv::Size(64, 64));
    EXPECT_EQ(cropped.type(), CV_8UC3);
    EXPECT_EQ(cropped.at<cv::Vec3b>(32, 32), cv::Vec3b(10, 20, 30));
}

TEST(
    FaceCropTest, KeepsSquareInputAtModelSize
)
{
    FaceCrop crop(FaceDetector(emotionFaceDetectorModelPath()), 64);
    const cv::Mat image(64, 64, CV_8UC3, cv::Scalar(40, 50, 60));

    const auto cropped = crop(image);

    ASSERT_EQ(cropped.size(), image.size());
    EXPECT_EQ(cropped.at<cv::Vec3b>(32, 32), image.at<cv::Vec3b>(32, 32));
}
