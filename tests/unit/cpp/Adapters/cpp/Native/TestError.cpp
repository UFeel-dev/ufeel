#include "ufeel/Model/Status.hpp"
#include "ufeel/Native/Error.hpp"

#include <gtest/gtest.h>
#include <string>

TEST(
    ErrorTest, DescribesTheFailedOperation
)
{
    const ufeel::Error error(ufeel::Status::InvalidState, "begin calibration");

    EXPECT_EQ(error.status(), ufeel::Status::InvalidState);
    EXPECT_EQ(
        std::string(error.what()),
        "Failed to begin calibration: The processor is not in a state that permits this "
        "operation. (status 6)."
    );
}

TEST(
    ErrorTest, UsesTheGivenReason
)
{
    const ufeel::Error error(ufeel::Status::InternalError, "read results", "unknown status 42");

    EXPECT_EQ(std::string(error.what()), "Failed to read results: unknown status 42 (status 9).");
}
