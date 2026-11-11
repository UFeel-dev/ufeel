#include "internal/Memory.hpp"

#include <cstdlib>
#include <cstring>
#include <gtest/gtest.h>
#include <string>

namespace
{

auto failAllocation(
    std::size_t size
) -> void*
{
    static_cast<void>(size);
    return nullptr;
}

} // namespace

TEST(
    MemoryTest, DuplicatesUtf8StringAndNullTerminatesIt
)
{
    const std::string value = "emotion";
    auto* duplicate = ufeel::internal::duplicateUtf8(value);

    ASSERT_NE(duplicate, nullptr);
    EXPECT_EQ(std::memcmp(duplicate, value.c_str(), value.size() + 1), 0);
    std::free(duplicate);
}

TEST(
    MemoryTest, ReturnsNullWhenAllocationFails
)
{
    EXPECT_EQ(ufeel::internal::duplicateUtf8("text", failAllocation), nullptr);
    EXPECT_EQ(ufeel::internal::duplicateUtf8("text", nullptr), nullptr);
}
