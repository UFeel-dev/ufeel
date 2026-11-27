#include "ufeel/Model/Status.hpp"

#include <gtest/gtest.h>

TEST(
    StatusTest, NamesStatuses
)
{
    EXPECT_EQ(ufeel::toString(ufeel::Status::Ok), "Ok");
    EXPECT_EQ(ufeel::toString(ufeel::Status::InvalidFrame), "InvalidFrame");
    EXPECT_EQ(ufeel::toString(ufeel::Status::UnsupportedFeature), "UnsupportedFeature");
}

TEST(
    StatusTest, DescribesStatuses
)
{
    EXPECT_EQ(ufeel::describe(ufeel::Status::InvalidFrame), "The frame is unavailable or invalid.");
    EXPECT_EQ(
        ufeel::describe(ufeel::Status::UnsupportedFeature),
        "The feature is not built into this library."
    );
}

TEST(
    StatusTest, HandlesUnknownStatuses
)
{
    const auto unknown = static_cast<ufeel::Status>(200);

    EXPECT_EQ(ufeel::toString(unknown), "Unknown");
    EXPECT_EQ(ufeel::describe(unknown), "An unknown native-library error occurred.");
}
