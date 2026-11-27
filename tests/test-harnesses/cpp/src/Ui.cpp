#include "TestHarness/Ui.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <raylib.h>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ui
{

namespace
{

constexpr float defaultFontSize = 10.0F;
constexpr float cornerRadius = 10.0F;
constexpr float controlRadius = 4.0F;
constexpr int cornerSegments = 8;
constexpr float itemInset = 8.0F;
constexpr float segmentGap = 4.0F;

auto activeClip() -> std::optional<Rectangle>&
{
    static std::optional<Rectangle> clip;
    return clip;
}

auto isPointerOver(
    Rectangle bounds
) -> bool
{
    const Vector2 pointer = GetMousePosition();
    const auto& clip = activeClip();
    return CheckCollisionPointRec(pointer, bounds) &&
           (!clip || CheckCollisionPointRec(pointer, *clip));
}

auto spacingFor(
    float size
) -> float
{
    return size / defaultFontSize;
}

auto measureAt(
    std::string_view text, float size
) -> float
{
    return MeasureTextEx(GetFontDefault(), std::string(text).c_str(), size, spacingFor(size)).x;
}

void draw(
    std::string_view text, Vector2 position, float size, Color color
)
{
    DrawTextEx(
        GetFontDefault(), std::string(text).c_str(), position, size, spacingFor(size), color
    );
}

auto roundness(
    Rectangle bounds, float radius
) -> float
{
    const float shortest = std::min(bounds.width, bounds.height);
    return shortest <= 0.0F ? 0.0F : std::min(1.0F, 2.0F * radius / shortest);
}

void appendWrapped(
    std::string_view paragraph, float width, std::vector<std::string>& lines
)
{
    std::string line;
    std::size_t start = 0;
    while (start <= paragraph.size())
    {
        const std::size_t end = std::min(paragraph.find(' ', start), paragraph.size());
        const std::string_view word = paragraph.substr(start, end - start);
        std::string candidate = line.empty() ? std::string(word) : line + " " + std::string(word);

        if (!line.empty() && measure(candidate) > width)
        {
            lines.push_back(std::move(line));
            line = std::string(word);
        }
        else
        {
            line = std::move(candidate);
        }
        start = end + 1;
    }
    lines.push_back(std::move(line));
}

auto wrap(
    std::string_view text, float width
) -> std::vector<std::string>
{
    std::vector<std::string> lines;
    std::size_t start = 0;
    while (start <= text.size())
    {
        const std::size_t end = std::min(text.find('\n', start), text.size());
        appendWrapped(text.substr(start, end - start), width, lines);
        start = end + 1;
    }
    return lines;
}

void popCodepoint(
    std::string& text
)
{
    constexpr unsigned char continuationMask = 0xC0;
    constexpr unsigned char continuationByte = 0x80;
    while (!text.empty() &&
           (static_cast<unsigned char>(text.back()) & continuationMask) == continuationByte)
    {
        text.pop_back();
    }
    if (!text.empty())
    {
        text.pop_back();
    }
}

auto ellipsize(
    std::string_view text, float width
) -> std::string
{
    std::string fitted(text);
    if (measure(fitted) <= width)
    {
        return fitted;
    }

    while (!fitted.empty() && measure(fitted + "...") > width)
    {
        popCodepoint(fitted);
    }
    return fitted + "...";
}

auto centeredIn(
    Rectangle bounds, float width, float height
) -> Vector2
{
    return {
        .x = bounds.x + ((bounds.width - width) / 2.0F),
        .y = bounds.y + ((bounds.height - height) / 2.0F),
    };
}

void drawControl(
    Rectangle bounds, std::string_view label, bool pressed, bool hovered, bool enabled
)
{
    Color fill = color::button;
    Color outline = color::border;
    if (pressed)
    {
        fill = color::buttonPressed;
        outline = color::borderPressed;
    }
    else if (hovered)
    {
        fill = color::buttonHover;
        outline = color::borderHover;
    }

    const float corner = roundness(bounds, controlRadius);
    DrawRectangleRounded(bounds, corner, cornerSegments, fill);
    DrawRectangleRoundedLinesEx(bounds, corner, cornerSegments, 1.0F, outline);
    draw(
        label,
        centeredIn(bounds, measure(label), fontSize),
        fontSize,
        enabled ? color::text : color::mutedText
    );
}

} // namespace

Window::Window(
    int width, int height, const char* title
)
{
    constexpr int minimumWidth = 900;
    constexpr int minimumHeight = 600;
    constexpr int framesPerSecond = 60;

    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(
        static_cast<unsigned int>(FLAG_WINDOW_RESIZABLE) |
        static_cast<unsigned int>(FLAG_VSYNC_HINT) | static_cast<unsigned int>(FLAG_MSAA_4X_HINT)
    );
    InitWindow(width, height, title);
    if (!IsWindowReady())
    {
        throw std::runtime_error("Cannot open a window, check that a display is available.");
    }

    SetWindowMinSize(minimumWidth, minimumHeight);
    SetExitKey(KEY_NULL);
    SetTargetFPS(framesPerSecond);
}

Window::~Window()
{
    CloseWindow();
}

Clip::Clip(
    Rectangle bounds
)
{
    BeginScissorMode(
        static_cast<int>(bounds.x),
        static_cast<int>(bounds.y),
        static_cast<int>(bounds.width),
        static_cast<int>(bounds.height)
    );
    activeClip() = bounds;
}

Clip::~Clip()
{
    EndScissorMode();
    activeClip().reset();
}

auto measure(
    std::string_view text
) -> float
{
    return measureAt(text, fontSize);
}

auto textHeight(
    std::string_view text, float width
) -> float
{
    return static_cast<float>(wrap(text, width).size()) * lineHeight;
}

auto text(
    std::string_view text, Vector2 position, float width, Color color
) -> float
{
    const auto lines = wrap(text, width);
    for (std::size_t index = 0; index < lines.size(); ++index)
    {
        draw(
            lines.at(index),
            {.x = position.x, .y = position.y + (static_cast<float>(index) * lineHeight)},
            fontSize,
            color
        );
    }
    return static_cast<float>(lines.size()) * lineHeight;
}

void title(
    std::string_view text, Vector2 position, Color color
)
{
    draw(text, position, fontSize, color);
    draw(text, {.x = position.x + 1.0F, .y = position.y}, fontSize, color);
}

void centeredText(
    std::string_view text, float y, float size, Color color
)
{
    const float x = (static_cast<float>(GetScreenWidth()) - measureAt(text, size)) / 2.0F;
    draw(text, {.x = x, .y = y}, size, color);
}

void card(
    Rectangle bounds, Color color
)
{
    DrawRectangleRounded(bounds, roundness(bounds, cornerRadius), cornerSegments, color);
}

auto textCard(
    std::string_view title, std::span<const Paragraph> paragraphs, Vector2 position, float width
) -> float
{
    const float innerWidth = width - (2.0F * padding);
    float height = (2.0F * padding) + lineHeight;
    for (const Paragraph& paragraph : paragraphs)
    {
        height += spacing + textHeight(paragraph.text, innerWidth);
    }

    card({.x = position.x, .y = position.y, .width = width, .height = height});
    float y = position.y + padding;
    ui::title(title, {.x = position.x + padding, .y = y});
    y += lineHeight;
    for (const Paragraph& paragraph : paragraphs)
    {
        y += spacing;
        y += text(paragraph.text, {.x = position.x + padding, .y = y}, innerWidth, paragraph.color);
    }
    return height;
}

void marker(
    Vector2 center, float radius, Color fill, Color stroke
)
{
    constexpr float strokeWidth = 3.0F;
    DrawCircleV(center, radius, stroke);
    DrawCircleV(center, radius - strokeWidth, fill);
}

auto button(
    Rectangle bounds, std::string_view label, bool enabled
) -> bool
{
    const bool hovered = enabled && isPointerOver(bounds);
    drawControl(bounds, label, hovered && IsMouseButtonDown(MOUSE_BUTTON_LEFT), hovered, enabled);
    return hovered && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}

auto segmented(
    Rectangle bounds, std::span<const std::string_view> labels, std::size_t selected, bool enabled
) -> std::optional<std::size_t>
{
    const float width = (bounds.width - (static_cast<float>(labels.size() - 1) * segmentGap)) /
                        static_cast<float>(labels.size());

    std::optional<std::size_t> clicked;
    std::size_t index = 0;
    for (const std::string_view label : labels)
    {
        const Rectangle segment{
            .x = bounds.x + (static_cast<float>(index) * (width + segmentGap)),
            .y = bounds.y,
            .width = width,
            .height = bounds.height,
        };
        const bool hovered = enabled && isPointerOver(segment);
        drawControl(segment, label, index == selected, hovered, enabled);
        if (hovered && IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        {
            clicked = index;
        }
        ++index;
    }
    return clicked;
}

auto listItem(
    Rectangle bounds, std::string_view label, bool selected, bool enabled
) -> bool
{
    const bool hovered = enabled && isPointerOver(bounds);
    if (selected || hovered)
    {
        DrawRectangleRounded(
            bounds,
            roundness(bounds, controlRadius),
            cornerSegments,
            selected ? color::buttonHover : color::rowHover
        );
    }

    draw(
        ellipsize(label, bounds.width - (2.0F * itemInset)),
        {.x = bounds.x + itemInset, .y = centeredIn(bounds, 0.0F, fontSize).y},
        fontSize,
        color::text
    );

    return hovered && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}

} // namespace ui
