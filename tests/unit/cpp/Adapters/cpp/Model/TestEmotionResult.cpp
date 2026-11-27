#include "ufeel/Model/EmotionResult.hpp"

#include <gtest/gtest.h>
#include <limits>
#include <optional>

TEST(
    EmotionResultTest, FindsTheStrongestKnownEmotion
)
{
    const ufeel::EmotionResult result{
        .status = ufeel::EmotionStatus::Valid,
        .scores = {{"sad", 0.2F}, {"happy", 0.7F}, {"joy", 0.9F}},
    };

    EXPECT_TRUE(result.isValid());
    EXPECT_EQ(result.dominant(), ufeel::Emotion::Happy);
}

TEST(
    EmotionResultTest, HasNoDominantEmotionWithoutKnownScores
)
{
    const ufeel::EmotionResult empty{};
    const ufeel::EmotionResult unknown{
        .status = ufeel::EmotionStatus::Valid,
        .scores = {{"joy", 0.9F}},
    };

    EXPECT_FALSE(empty.isValid());
    EXPECT_EQ(empty.dominant(), std::nullopt);
    EXPECT_EQ(unknown.dominant(), std::nullopt);
}

TEST(
    EmotionResultTest, IgnoresScoresThatAreNotANumber
)
{
    const ufeel::EmotionResult result{
        .status = ufeel::EmotionStatus::Valid,
        .scores = {{"angry", std::numeric_limits<float>::quiet_NaN()}, {"sad", 0.1F}},
    };

    EXPECT_EQ(result.dominant(), ufeel::Emotion::Sad);
}

TEST(
    EmotionResultTest, ParsesLabelsIgnoringCase
)
{
    EXPECT_EQ(ufeel::parseEmotion("surprised"), ufeel::Emotion::Surprised);
    EXPECT_EQ(ufeel::parseEmotion("CONTEMPTUOUS"), ufeel::Emotion::Contemptuous);
    EXPECT_EQ(ufeel::parseEmotion("surprise"), std::nullopt);
    EXPECT_EQ(ufeel::parseEmotion(""), std::nullopt);
}

TEST(
    EmotionResultTest, NamesEmotions
)
{
    EXPECT_EQ(ufeel::toString(ufeel::Emotion::Fearful), "Fearful");
    EXPECT_EQ(ufeel::toString(static_cast<ufeel::Emotion>(200)), "Unknown");
}
