#include "Utils/FrameFormat.hpp"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/matx.hpp"
#include "opencv2/core/types.hpp"

#include <gtest/gtest.h>

TEST(
    FrameFormatTest, ExpandsGreyscaleToBgr
)
{
    const cv::Mat grey(2, 2, CV_8UC1, cv::Scalar(64));

    const auto bgr = toBgr(grey);

    ASSERT_EQ(bgr.type(), CV_8UC3);
    EXPECT_EQ(bgr.at<cv::Vec3b>(0, 0), cv::Vec3b(64, 64, 64));
}

TEST(
    FrameFormatTest, DropsAlphaFromBgra
)
{
    const cv::Mat bgra(2, 2, CV_8UC4, cv::Scalar(10, 20, 30, 40));

    const auto bgr = toBgr(bgra);

    ASSERT_EQ(bgr.type(), CV_8UC3);
    EXPECT_EQ(bgr.at<cv::Vec3b>(0, 0), cv::Vec3b(10, 20, 30));
}

TEST(
    FrameFormatTest, KeepsOtherFormatsUnchanged
)
{
    const cv::Mat bgr(2, 2, CV_8UC3, cv::Scalar(10, 20, 30));
    const cv::Mat twoChannel(2, 2, CV_8UC2, cv::Scalar(10, 20));

    EXPECT_EQ(toBgr(bgr).data, bgr.data);
    EXPECT_EQ(toBgr(twoChannel).data, twoChannel.data);
}
