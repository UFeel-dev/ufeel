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
