#ifndef TESTHARNESS_UI_HPP
#define TESTHARNESS_UI_HPP

#include <cstddef>
#include <optional>
#include <raylib.h>
#include <span>
#include <string>
#include <string_view>

namespace ui
{

namespace color
{

inline constexpr Color background{.r = 0xF4, .g = 0xF6, .b = 0xF8, .a = 0xFF};
inline constexpr Color text{.r = 0x17, .g = 0x20, .b = 0x2A, .a = 0xFF};
inline constexpr Color mutedText{.r = 0x68, .g = 0x77, .b = 0x85, .a = 0xFF};
inline constexpr Color lightText{.r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0xFF};
inline constexpr Color card{.r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0xFF};
inline constexpr Color cameraCard{.r = 0x17, .g = 0x1B, .b = 0x22, .a = 0xFF};
inline constexpr Color cameraView{.r = 0x0E, .g = 0x11, .b = 0x16, .a = 0xFF};
inline constexpr Color button{.r = 0xE7, .g = 0xED, .b = 0xF4, .a = 0xFF};
inline constexpr Color buttonHover{.r = 0xD8, .g = 0xE2, .b = 0xED, .a = 0xFF};
inline constexpr Color buttonPressed{.r = 0xC7, .g = 0xD5, .b = 0xE3, .a = 0xFF};
inline constexpr Color border{.r = 0xAA, .g = 0xB7, .b = 0xC4, .a = 0xFF};
inline constexpr Color borderHover{.r = 0x71, .g = 0x86, .b = 0x9B, .a = 0xFF};
inline constexpr Color borderPressed{.r = 0x53, .g = 0x6B, .b = 0x82, .a = 0xFF};
inline constexpr Color rowHover{.r = 0xEE, .g = 0xF2, .b = 0xF6, .a = 0xFF};
inline constexpr Color overlay{.r = 0x17, .g = 0x1B, .b = 0x22, .a = 0xE6};
inline constexpr Color gazeMarker{.r = 0xF2, .g = 0xC9, .b = 0x4C, .a = 0xFF};
inline constexpr Color calibrationTarget{.r = 0x35, .g = 0xD0, .b = 0x7F, .a = 0xFF};

} // namespace color

inline constexpr float fontSize = 20.0F;
inline constexpr float lineHeight = 24.0F;
inline constexpr float padding = 16.0F;
inline constexpr float spacing = 10.0F;

struct Paragraph
{
    std::string text;
    Color color = color::text;
};

class Window
{
public:

    Window(int width, int height, const char* title);
    ~Window();

    Window(const Window&) = delete;
    Window(Window&&) = delete;
    auto operator=(const Window&) -> Window& = delete;
    auto operator=(Window&&) -> Window& = delete;
};

class Clip
{
public:

    explicit Clip(Rectangle bounds);
    ~Clip();

    Clip(const Clip&) = delete;
    Clip(Clip&&) = delete;
    auto operator=(const Clip&) -> Clip& = delete;
    auto operator=(Clip&&) -> Clip& = delete;
};

[[nodiscard]] auto measure(std::string_view text) -> float;
[[nodiscard]] auto textHeight(std::string_view text, float width) -> float;

auto text(std::string_view text, Vector2 position, float width, Color color = color::text) -> float;
void title(std::string_view text, Vector2 position, Color color = color::text);
void centeredText(std::string_view text, float y, float size, Color color);

void card(Rectangle bounds, Color color = color::card);
auto textCard(
    std::string_view title, std::span<const Paragraph> paragraphs, Vector2 position, float width
) -> float;
void marker(Vector2 center, float radius, Color fill, Color stroke);

auto button(Rectangle bounds, std::string_view label, bool enabled) -> bool;
auto segmented(
    Rectangle bounds, std::span<const std::string_view> labels, std::size_t selected, bool enabled
) -> std::optional<std::size_t>;
auto listItem(Rectangle bounds, std::string_view label, bool selected, bool enabled) -> bool;

} // namespace ui

#endif // TESTHARNESS_UI_HPP
