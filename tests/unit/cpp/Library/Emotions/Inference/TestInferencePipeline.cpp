#include "Emotions/EmotionModel.hpp"
#include "Emotions/Inference/FaceCrop.hpp"
#include "Emotions/Inference/FaceDetector.hpp"
#include "Emotions/Inference/InferencePipeline.hpp"
#include "Library/Emotions/EmotionTestPaths.hpp"
#include "Library/TestSupport/Fixtures.hpp"
#include "torch/headeronly/core/DeviceType.h"

#include <cmath>
#include <gtest/gtest.h>

namespace
{

auto createPipeline() -> InferencePipeline
{
    return {
        EmotionModel("resnet34"),
        torch::kCPU,
        FaceCrop(FaceDetector(emotionFaceDetectorModelPath()), 32),
    };
}

} // namespace

TEST(
    InferencePipelineTest, ReturnsFiniteProbabilityForEveryEmotionOfAFace
)
{
    auto pipeline = createPipeline();

    const auto scores = pipeline.predict(loadFaceFrame());
    if (!scores)
    {
        FAIL() << "no face was found in the face fixture";
    }

    for (const auto score : *scores)
    {
        EXPECT_TRUE(std::isfinite(score));
        EXPECT_GE(score, 0.0F);
        EXPECT_LE(score, 1.0F);
    }
}

TEST(
    InferencePipelineTest, ReturnsNothingWithoutAFace
)
{
    auto pipeline = createPipeline();

    EXPECT_FALSE(pipeline.predict(makeBlankFrame()).has_value());
}
