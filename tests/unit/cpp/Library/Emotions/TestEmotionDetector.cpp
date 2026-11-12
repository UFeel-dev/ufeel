#include "Emotions/EmotionDetector.hpp"
#include "Emotions/EmotionLabels.hpp"
#include "Library/Emotions/EmotionTestPaths.hpp"
#include "Library/TestSupport/Fixtures.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/imgproc.hpp"

#include <algorithm>
#include <cmath>
#include <exception>
#include <gtest/gtest.h>

TEST(
    EmotionDetectorTest, DetectsHappinessOnFaceFixture
)
{
    EmotionDetector detector(emotionModelDirectory());
    const auto face = loadFaceFrame();
    ASSERT_FALSE(face.empty());

    const auto firstResult = detector.process(face);
    const auto cachedResult = detector.process(face);

    EXPECT_EQ(cachedResult, firstResult);
    ASSERT_TRUE(firstResult.contains("happy"));
    for (const auto& [label, score] : firstResult)
    {
        EXPECT_NE(std::ranges::find(kEmotionLabels, label), kEmotionLabels.end());
        EXPECT_TRUE(std::isfinite(score));
        EXPECT_GE(score, 0.0F);
        EXPECT_LE(score, 1.0F);
    }
}

TEST(
    EmotionDetectorTest, AcceptsGreyscaleAndBgraFrames
)
{
    EmotionDetector detector(emotionModelDirectory());
    cv::Mat bgra;
    cv::cvtColor(loadFaceFrame(), bgra, cv::COLOR_BGR2BGRA);

    EXPECT_TRUE(detector.process(makeGreyscaleFaceFrame()).contains("happy"));
    EXPECT_TRUE(detector.process(bgra).contains("happy"));
}

TEST(
    EmotionDetectorTest, ReturnsNoEmotionWithoutAFace
)
{
    EmotionDetector detector(emotionModelDirectory());

    EXPECT_TRUE(detector.process(makeBlankFrame()).empty());
    EXPECT_TRUE(detector.process(makeSmallFaceFrame()).empty());
}

TEST(
    EmotionDetectorTest, ThrowsWhenModelConfigurationIsMissing
)
{
    EXPECT_THROW(EmotionDetector("missing-emotion-model-directory"), std::exception);
}
