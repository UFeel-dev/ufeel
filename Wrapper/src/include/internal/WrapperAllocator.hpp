#ifndef INTERNAL_WRAPPERALLOCATOR_HPP
#define INTERNAL_WRAPPERALLOCATOR_HPP

#include "Memory.hpp"
#include "ufeel.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <new>
#include <string>

namespace ufeel::internal
{

class WrapperAllocator
{
public:

    WrapperAllocator() = default;
    virtual ~WrapperAllocator() = default;
    WrapperAllocator(const WrapperAllocator&) = delete;
    auto operator=(const WrapperAllocator&) -> WrapperAllocator& = delete;
    WrapperAllocator(WrapperAllocator&&) = delete;
    auto operator=(WrapperAllocator&&) -> WrapperAllocator& = delete;

    [[nodiscard]] virtual auto createFrame() noexcept -> UFeelFrame* = 0;
    [[nodiscard]] virtual auto allocateFrameData(std::size_t size) noexcept -> std::uint8_t* = 0;
    [[nodiscard]] virtual auto allocateEmotionPairs(std::size_t count) noexcept -> UFeelPair* = 0;
    [[nodiscard]] virtual auto duplicateString(const std::string& value) noexcept -> char* = 0;
};

class SystemWrapperAllocator final : public WrapperAllocator
{
public:

    [[nodiscard]] auto createFrame() noexcept -> UFeelFrame* override
    {
        return new (std::nothrow) UFeelFrame{};
    }

    [[nodiscard]] auto allocateFrameData(
        std::size_t size
    ) noexcept -> std::uint8_t* override
    {
        return static_cast<std::uint8_t*>(std::malloc(size));
    }

    [[nodiscard]] auto allocateEmotionPairs(
        std::size_t count
    ) noexcept -> UFeelPair* override
    {
        return new (std::nothrow) UFeelPair[count]{};
    }

    [[nodiscard]] auto duplicateString(
        const std::string& value
    ) noexcept -> char* override
    {
        return duplicateUtf8(value);
    }
};

inline auto defaultWrapperAllocator() -> WrapperAllocator&
{
    static SystemWrapperAllocator allocator;
    return allocator;
}

} // namespace ufeel::internal

#endif // INTERNAL_WRAPPERALLOCATOR_HPP
