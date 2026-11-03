#include "EyeTracking/IrisDetection/DetectionPostProcess.hpp"

#include <gtest/gtest.h>
#include <vector>

class TestDetectionPostProcess : public ::testing::Test
{
protected:

    my::DetectionPostProcess postProcess_;
};

TEST_F(
    TestDetectionPostProcess, ReturnsDefaultDetectionWhenNoScoreExceedsThreshold
)
{
    std::vector<float> rawBoxes(NUM_BOXES * NUM_COORD, 0.0f);
    std::vector<float> scores(NUM_BOXES, 0.0f);

    auto const detection = postProcess_.getHighestScoreDetection(rawBoxes, scores);

    EXPECT_FLOAT_EQ(detection.score, 0.0f);
    EXPECT_EQ(detection.classId, -1);
}

TEST_F(
    TestDetectionPostProcess, ReturnsDefaultDetectionWhenScoreEqualsThreshold
)
{
    std::vector<float> rawBoxes(NUM_BOXES * NUM_COORD, 0.0f);
    std::vector<float> scores(NUM_BOXES, 0.0f);
    scores[0] = MIN_THRESHOLD;

    auto const detection = postProcess_.getHighestScoreDetection(rawBoxes, scores);

    EXPECT_FLOAT_EQ(detection.score, 0.0f);
    EXPECT_EQ(detection.classId, -1);
}

TEST_F(
    TestDetectionPostProcess, ReturnsDetectionWhenScoreExceedsThreshold
)
{
    std::vector<float> rawBoxes(NUM_BOXES * NUM_COORD, 0.0f);
    std::vector<float> scores(NUM_BOXES, 0.0f);
    scores[0] = 0.9f;

    auto const detection = postProcess_.getHighestScoreDetection(rawBoxes, scores);

    EXPECT_FLOAT_EQ(detection.score, 0.9f);
    EXPECT_EQ(detection.classId, CLASS_ID);
}

TEST_F(
    TestDetectionPostProcess, ReturnsDetectionWithHighestScore
)
{
    std::vector<float> rawBoxes(NUM_BOXES * NUM_COORD, 0.0f);
    std::vector<float> scores(NUM_BOXES, 0.0f);

    scores[0] = 0.8f;
    scores[1] = 0.95f;
    scores[2] = 0.85f;

    auto const detection = postProcess_.getHighestScoreDetection(rawBoxes, scores);

    EXPECT_FLOAT_EQ(detection.score, 0.95f);
    EXPECT_EQ(detection.classId, CLASS_ID);
}
