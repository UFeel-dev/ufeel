#include "../EmotionTestPaths.hpp"
#include "Emotions/Inference/FaceDetector.hpp"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"

#include <gtest/gtest.h>

TEST(
    FaceDetectorTest, ThrowsWhenModelDoesNotExist
)
{
    EXPECT_ANY_THROW(FaceDetector detector("missing-face-detector.onnx"));
}

TEST(
    FaceDetectorTest, LoadsModelAndReturnsNoDetectionForBlankImage
)
{
    FaceDetector detector(emotionFaceDetectorModelPath());
    const cv::Mat image(240, 320, CV_8UC3, cv::Scalar::all(0));

    EXPECT_FALSE(detector.detect(image).has_value());
}
