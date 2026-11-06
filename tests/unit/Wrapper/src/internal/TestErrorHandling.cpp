#include "internal/ErrorHandling.hpp"

#include <gtest/gtest.h>
#include <stdexcept>

TEST(
    ErrorHandlingTest, GuardedReturnsFunctionResult
)
{
    const int result = ufeel::internal::guarded("operation", -1, [] { return 42; });

    EXPECT_EQ(result, 42);
}

TEST(
    ErrorHandlingTest, GuardedReturnsFallbackForStandardException
)
{
    const int result = ufeel::internal::guarded("operation", -1, []() -> int {
        throw std::runtime_error("failure");
    });

    EXPECT_EQ(result, -1);
}

TEST(
    ErrorHandlingTest, GuardedReturnsFallbackForUnknownException
)
{
    const int result = ufeel::internal::guarded("operation", -1, []() -> int { throw 1; });

    EXPECT_EQ(result, -1);
}

TEST(
    ErrorHandlingTest, GuardedVoidRunsFunction
)
{
    bool called = false;

    ufeel::internal::guardedVoid("operation", [&called] { called = true; });

    EXPECT_TRUE(called);
}

TEST(
    ErrorHandlingTest, GuardedVoidHandlesExceptions
)
{
    EXPECT_NO_THROW(ufeel::internal::guardedVoid("operation", [] {
        throw std::runtime_error("failure");
    }));
    EXPECT_NO_THROW(ufeel::internal::guardedVoid("operation", [] { throw 1; }));
}
