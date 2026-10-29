#include "SpeechToText/AudioCapture.hpp"

#include <gtest/gtest.h>
#include <vector>

TEST(
    AudioCaptureTest, ReadReturnsFalseBeforeStart
)
{
    AudioCapture capture;
    std::vector<int16_t> buffer;

    EXPECT_FALSE(capture.read(buffer));
    EXPECT_TRUE(buffer.empty());
}

TEST(
    AudioCaptureTest, StopWithoutStartingDoesNotCrash
)
{
    AudioCapture capture;

    EXPECT_NO_THROW(capture.stop());
}

TEST(
    AudioCaptureTest, RepeatedStopDoesNotCrash
)
{
    AudioCapture capture;

    capture.stop();
    EXPECT_NO_THROW(capture.stop());
}

TEST(
    AudioCaptureTest, ReadReturnsFalseWhenStoppedAndNoDataAvailable
)
{
    AudioCapture capture;
    std::vector<int16_t> buffer;

    capture.stop();

    EXPECT_FALSE(capture.read(buffer));
}

TEST(
    AudioCaptureTest, MultipleReadsBeforeStartReturnFalse
)
{
    AudioCapture capture;
    std::vector<int16_t> buffer;

    EXPECT_FALSE(capture.read(buffer));
    EXPECT_FALSE(capture.read(buffer));
    EXPECT_FALSE(capture.read(buffer));
}
