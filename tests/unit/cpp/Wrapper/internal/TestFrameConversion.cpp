#include "internal/FrameConversion.hpp"
#include "opencv2/core/hal/interface.h"
#include "ufeel.h"

#include <array>
#include <cstdint>
#include <gtest/gtest.h>

using ufeel::internal::convertFrame;
using ufeel::internal::FrameCopyPolicy;

TEST(
    FrameConversionTest, ReturnsEmptyForNullFrameOrData
)
{
    EXPECT_TRUE(convertFrame(nullptr).empty());

    const UFeelFrame frame{
        .data = nullptr,
        .width = 1,
        .height = 1,
        .channels = 1,
        .stride = 1,
        .dataSize = 1,
    };

    EXPECT_TRUE(convertFrame(&frame).empty());
}

TEST(
    FrameConversionTest, ReturnsEmptyForInvalidDimensionsChannelsOrStride
)
{
    std::array<std::uint8_t, 4> pixels{};
    UFeelFrame frame{
        .data = pixels.data(),
        .width = 2,
        .height = 2,
        .channels = 1,
        .stride = 2,
        .dataSize = pixels.size(),
    };

    frame.width = 0;
    EXPECT_TRUE(convertFrame(&frame).empty());
    frame.width = 2;

    frame.height = -1;
    EXPECT_TRUE(convertFrame(&frame).empty());
    frame.height = 2;

    frame.channels = 0;
    EXPECT_TRUE(convertFrame(&frame).empty());
    frame.channels = CV_CN_MAX + 1;
    EXPECT_TRUE(convertFrame(&frame).empty());
    frame.channels = 1;

    frame.stride = 0;
    EXPECT_TRUE(convertFrame(&frame).empty());
}

TEST(
    FrameConversionTest, ReturnsEmptyWhenStrideOrBufferIsTooSmall
)
{
    std::array<std::uint8_t, 4> pixels{};
    UFeelFrame frame{
        .data = pixels.data(),
        .width = 2,
        .height = 2,
        .channels = 1,
        .stride = 1,
        .dataSize = pixels.size(),
    };

    EXPECT_TRUE(convertFrame(&frame).empty());

    frame.stride = 2;
    frame.dataSize = 3;
    EXPECT_TRUE(convertFrame(&frame).empty());
}

TEST(
    FrameConversionTest, PreservesPaddedRowsAndCopiesByDefault
)
{
    std::array<std::uint8_t, 6> pixels{1, 2, 99, 3, 4, 99};
    const UFeelFrame frame{
        .data = pixels.data(),
        .width = 2,
        .height = 2,
        .channels = 1,
        .stride = 3,
        .dataSize = pixels.size(),
    };

    auto converted = convertFrame(&frame);

    ASSERT_FALSE(converted.empty());
    EXPECT_EQ(converted.rows, 2);
    EXPECT_EQ(converted.cols, 2);
    EXPECT_EQ(converted.at<std::uint8_t>(0, 0), 1U);
    EXPECT_EQ(converted.at<std::uint8_t>(1, 1), 4U);
    EXPECT_NE(converted.data, frame.data);

    converted.at<std::uint8_t>(0, 0) = 8;
    EXPECT_EQ(pixels.at(0), 1U);
}

TEST(
    FrameConversionTest, BorrowsInputWhenRequested
)
{
    std::array<std::uint8_t, 4> pixels{1, 2, 3, 4};
    const UFeelFrame frame{
        .data = pixels.data(),
        .width = 2,
        .height = 2,
        .channels = 1,
        .stride = 2,
        .dataSize = pixels.size(),
    };

    auto converted = convertFrame(&frame, FrameCopyPolicy::Borrow);

    ASSERT_FALSE(converted.empty());
    EXPECT_EQ(converted.data, frame.data);
    converted.at<std::uint8_t>(0, 0) = 8;
    EXPECT_EQ(pixels.at(0), 8U);
}
