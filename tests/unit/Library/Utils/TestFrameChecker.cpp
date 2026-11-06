#include "Utils/FrameChecker.hpp"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"

#include <gtest/gtest.h>

TEST(
    FrameCheckerTest, HasNoPreviousFrameInitially
)
{
    const FrameChecker<int> checker;

    EXPECT_FALSE(checker.hasPreviousFrame());
    EXPECT_FALSE(checker.isFrameIdentical(cv::Mat(2, 2, CV_8UC1)));
    EXPECT_EQ(checker.getResult(), 0);
}

TEST(
    FrameCheckerTest, DetectsIdenticalFramesAndSnapshotsInput
)
{
    FrameChecker<int> checker;
    cv::Mat frame(2, 2, CV_8UC1, cv::Scalar(7));

    checker.setFrame(frame);
    EXPECT_TRUE(checker.hasPreviousFrame());
    EXPECT_TRUE(checker.isFrameIdentical(frame));
    EXPECT_FALSE(checker.isFrameIdentical(cv::Mat{}));

    frame.setTo(cv::Scalar(9));
    EXPECT_FALSE(checker.isFrameIdentical(frame));
}

TEST(
    FrameCheckerTest, DetectsPixelSizeAndTypeChanges
)
{
    FrameChecker<int> checker;
    checker.setFrame(cv::Mat(2, 2, CV_8UC1, cv::Scalar(7)));

    EXPECT_FALSE(checker.isFrameIdentical(cv::Mat(3, 2, CV_8UC1, cv::Scalar(7))));
    EXPECT_FALSE(checker.isFrameIdentical(cv::Mat(2, 2, CV_8UC3, cv::Scalar(7))));
    EXPECT_FALSE(checker.isFrameIdentical(cv::Mat{}));
}

TEST(
    FrameCheckerTest, EmptyFrameClearsPreviousFrame
)
{
    FrameChecker<int> checker;
    checker.setFrame(cv::Mat(2, 2, CV_8UC1, cv::Scalar(7)));

    checker.setFrame(cv::Mat{});

    EXPECT_FALSE(checker.hasPreviousFrame());
    EXPECT_FALSE(checker.isFrameIdentical(cv::Mat(2, 2, CV_8UC1, cv::Scalar(7))));
}

TEST(
    FrameCheckerTest, StoresAndReplacesAssociatedResult
)
{
    FrameChecker<int> checker;

    checker.setResult(42);
    EXPECT_EQ(checker.getResult(), 42);

    checker.setResult(84);
    EXPECT_EQ(checker.getResult(), 84);
}
