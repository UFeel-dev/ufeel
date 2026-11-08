#include "Emotions/EmotionLabels.hpp"
#include "Emotions/Inference/HeadCalibration.hpp"

#include <gtest/gtest.h>
#include <map>
#include <stdexcept>
#include <string>

TEST(
    HeadCalibrationTest, SelectsScoresAboveTheDefaultThreshold
)
{
    const HeadCalibration calibration;
    const EmotionScores scores{0.5F, 0.6F, 0.4F, 0.7F, 0.5F, 0.9F, 0.49F, 0.5F};

    const auto selected = calibration.select(scores);

    EXPECT_EQ(selected.size(), 3U);
    EXPECT_FLOAT_EQ(selected.at("contemptuous"), 0.6F);
    EXPECT_FLOAT_EQ(selected.at("fearful"), 0.7F);
    EXPECT_FLOAT_EQ(selected.at("neutral"), 0.9F);
    EXPECT_EQ(selected.count("angry"), 0U);
}

TEST(
    HeadCalibrationTest, AppliesOverridesAndKeepsDefaultsForOtherEmotions
)
{
    const HeadCalibration calibration({{"angry", 0.2F}, {"happy", 0.8F}});
    const EmotionScores scores{0.2F, 0.49F, 0.6F, 0.4F, 0.8F, 0.51F, 0.3F, 0.5F};

    const auto selected = calibration.select(scores);

    ASSERT_EQ(selected.size(), 2U);
    EXPECT_FLOAT_EQ(selected.at("neutral"), 0.51F);
    EXPECT_FLOAT_EQ(selected.at("disgusted"), 0.6F);
}

TEST(
    HeadCalibrationTest, RejectsUnknownEmotionThresholds
)
{
    EXPECT_THROW(HeadCalibration({{"unknown", 0.5F}}), std::invalid_argument);
}
