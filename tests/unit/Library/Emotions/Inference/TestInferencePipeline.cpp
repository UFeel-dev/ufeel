#include "../EmotionTestPaths.hpp"
#include "Emotions/EmotionModel.hpp"
#include "Emotions/Inference/FaceCrop.hpp"
#include "Emotions/Inference/FaceDetector.hpp"
#include "Emotions/Inference/InferencePipeline.hpp"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"
#include "torch/headeronly/core/DeviceType.h"

#include <cmath>
#include <gtest/gtest.h>

TEST(
    InferencePipelineTest, ReturnsFiniteProbabilityForEveryEmotion
)
{
    InferencePipeline pipeline(
        EmotionModel("resnet34"),
        torch::kCPU,
        FaceCrop(FaceDetector(emotionFaceDetectorModelPath()), 32)
    );
    const cv::Mat image(80, 120, CV_8UC3, cv::Scalar(10, 20, 30));

    const auto scores = pipeline.predict(image);

    for (const auto score : scores)
    {
        EXPECT_TRUE(std::isfinite(score));
        EXPECT_GE(score, 0.0F);
        EXPECT_LE(score, 1.0F);
    }
}
