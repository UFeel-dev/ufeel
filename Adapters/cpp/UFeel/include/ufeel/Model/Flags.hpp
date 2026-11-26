#ifndef UFEEL_MODEL_FLAGS_HPP
#define UFEEL_MODEL_FLAGS_HPP

#include <type_traits>

namespace ufeel
{

template <typename Enum> struct EnableFlags : std::false_type
{
};

template <typename Enum>
concept Flags = std::is_enum_v<Enum> && EnableFlags<Enum>::value;

template <Flags Enum>
[[nodiscard]] constexpr auto operator|(
    Enum left, Enum right
) noexcept -> Enum
{
    using Underlying = std::underlying_type_t<Enum>;
    return static_cast<Enum>(static_cast<Underlying>(left) | static_cast<Underlying>(right));
}

template <Flags Enum>
[[nodiscard]] constexpr auto operator&(
    Enum left, Enum right
) noexcept -> Enum
{
    using Underlying = std::underlying_type_t<Enum>;
    return static_cast<Enum>(static_cast<Underlying>(left) & static_cast<Underlying>(right));
}

template <Flags Enum>
[[nodiscard]] constexpr auto operator~(
    Enum value
) noexcept -> Enum
{
    using Underlying = std::underlying_type_t<Enum>;
    return static_cast<Enum>(~static_cast<Underlying>(value));
}

template <Flags Enum>
constexpr auto operator|=(
    Enum& left, Enum right
) noexcept -> Enum&
{
    left = left | right;
    return left;
}

template <Flags Enum>
constexpr auto operator&=(
    Enum& left, Enum right
) noexcept -> Enum&
{
    left = left & right;
    return left;
}

template <Flags Enum>
[[nodiscard]] constexpr auto hasAny(
    Enum value, Enum flags
) noexcept -> bool
{
    return (value & flags) != Enum{};
}

template <Flags Enum>
[[nodiscard]] constexpr auto hasAll(
    Enum value, Enum flags
) noexcept -> bool
{
    return (value & flags) == flags;
}

} // namespace ufeel

#endif // UFEEL_MODEL_FLAGS_HPP
