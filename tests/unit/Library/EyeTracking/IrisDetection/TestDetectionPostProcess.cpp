#include "EyeTracking/IrisDetection/DetectionPostProcess.hpp"

#include <cstddef>
#include <gtest/gtest.h>
#include <vector>

namespace
{

constexpr auto kRawBoxValueCount =
    static_cast<std::size_t>(NUM_BOXES) * static_cast<std::size_t>(NUM_COORD);

class TestDetectionPostProcess : public ::testing::Test
{
protected:

    my::DetectionPostProcess postProcess_;
};

} // namespace

TEST_F(
    TestDetectionPostProcess, ReturnsDefaultDetectionWhenNoScoreExceedsThreshold
)
{
    const std::vector<float> rawBoxes(kRawBoxValueCount, 0.0f);
    const std::vector<float> scores(NUM_BOXES, 0.0f);

    auto const detection = postProcess_.getHighestScoreDetection(rawBoxes, scores);

    EXPECT_FLOAT_EQ(detection.score, 0.0f);
    EXPECT_EQ(detection.classId, -1);
}

TEST_F(
    TestDetectionPostProcess, ReturnsDefaultDetectionWhenScoreEqualsThreshold
)
{
    const std::vector<float> rawBoxes(kRawBoxValueCount, 0.0f);
    std::vector<float> scores(NUM_BOXES, 0.0f);
    scores.at(0) = MIN_THRESHOLD;

    auto const detection = postProcess_.getHighestScoreDetection(rawBoxes, scores);

    EXPECT_FLOAT_EQ(detection.score, 0.0f);
    EXPECT_EQ(detection.classId, -1);
}

TEST_F(
    TestDetectionPostProcess, ReturnsDetectionWhenScoreExceedsThreshold
)
{
    const std::vector<float> rawBoxes(kRawBoxValueCount, 0.0f);
    std::vector<float> scores(NUM_BOXES, 0.0f);
    scores.at(0) = 0.9f;

    auto const detection = postProcess_.getHighestScoreDetection(rawBoxes, scores);

    EXPECT_FLOAT_EQ(detection.score, 0.9f);
    EXPECT_EQ(detection.classId, CLASS_ID);
}

TEST_F(
    TestDetectionPostProcess, ReturnsDetectionWithHighestScore
)
{
    const std::vector<float> rawBoxes(kRawBoxValueCount, 0.0f);
    std::vector<float> scores(NUM_BOXES, 0.0f);

    scores.at(0) = 0.8f;
    scores.at(1) = 0.95f;
    scores.at(2) = 0.85f;

    auto const detection = postProcess_.getHighestScoreDetection(rawBoxes, scores);

    EXPECT_FLOAT_EQ(detection.score, 0.95f);
    EXPECT_EQ(detection.classId, CLASS_ID);
}

TEST_F(
    TestDetectionPostProcess, DecodesBoundingBoxUsingAnchorAndOffsets
)
{
    std::vector<float> rawBoxes(kRawBoxValueCount, 0.0f);
    std::vector<float> scores(NUM_BOXES, 0.0f);

    rawBoxes.at(0) = 0.0f;
    rawBoxes.at(1) = 0.0f;
    rawBoxes.at(2) = 128.0f;
    rawBoxes.at(3) = 128.0f;

    scores.at(0) = 0.9f;

    auto const detection = postProcess_.getHighestScoreDetection(rawBoxes, scores);

    EXPECT_FLOAT_EQ(detection.roi.x, -0.46875f);
    EXPECT_FLOAT_EQ(detection.roi.y, -0.46875f);
    EXPECT_FLOAT_EQ(detection.roi.width, 1.0f);
    EXPECT_FLOAT_EQ(detection.roi.height, 1.0f);
}

TEST_F(
    TestDetectionPostProcess, KeepsFirstDetectionWhenScoresAreEqual
)
{
    std::vector<float> rawBoxes(kRawBoxValueCount, 0.0f);
    std::vector<float> scores(NUM_BOXES, 0.0f);

    scores.at(0) = 0.9f;
    scores.at(1) = 0.9f;

    rawBoxes.at(NUM_COORD + 2) = 64.0f;

    auto const detection = postProcess_.getHighestScoreDetection(rawBoxes, scores);

    EXPECT_FLOAT_EQ(detection.score, 0.9f);
    EXPECT_FLOAT_EQ(detection.roi.width, 0.0f);
}
