#include "Emotions/EmotionDetector.hpp"
#include "Emotions/EmotionLabels.hpp"
#include "Library/Emotions/EmotionTestPaths.hpp"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"

#include <algorithm>
#include <cmath>
#include <exception>
#include <gtest/gtest.h>

TEST(
    EmotionDetectorTest, LoadsModelsAndProcessesFrames
)
{
    EmotionDetector detector(emotionModelDirectory());
    const cv::Mat image(240, 320, CV_8UC3, cv::Scalar(10, 20, 30));

    const auto firstResult = detector.process(image);
    const auto cachedResult = detector.process(image);

    EXPECT_EQ(cachedResult, firstResult);
    for (const auto& [label, score] : firstResult)
    {
        EXPECT_NE(std::ranges::find(kEmotionLabels, label), kEmotionLabels.end());
        EXPECT_TRUE(std::isfinite(score));
        EXPECT_GE(score, 0.0F);
        EXPECT_LE(score, 1.0F);
    }
}

TEST(
    EmotionDetectorTest, ThrowsWhenModelConfigurationIsMissing
)
{
    EXPECT_THROW(EmotionDetector("missing-emotion-model-directory"), std::exception);
}
