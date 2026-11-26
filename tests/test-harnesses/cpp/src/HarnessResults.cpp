#include "TestHarness/Harness.hpp"
#include "TestHarness/Ui.hpp"
#include "ufeel/Model/EmotionResult.hpp"
#include "ufeel/Model/EyeTrackingResult.hpp"
#include "ufeel/Text/Text.hpp"

#include <algorithm>
#include <array>
#include <raylib.h>
#include <string>

namespace
{

constexpr float scrollStep = 40.0F;
constexpr float markerRadius = 12.0F;

auto emotionsText(
    const ufeel::EmotionResult& emotions
) -> std::string
{
    if (!emotions.isValid())
    {
        return ufeel::text::emotions(emotions);
    }
    return "Dominant: " + ufeel::text::dominantEmotion(emotions) + "\n" +
           ufeel::text::emotions(emotions);
}

auto directionsText(
    const ufeel::EyeTrackingResult& eyeTracking
) -> std::string
{
    if (!eyeTracking.isTracked())
    {
        return ufeel::text::directions(eyeTracking);
    }
    return ufeel::text::directions(eyeTracking) + "\n" + ufeel::text::eyes(eyeTracking);
}

} // namespace

void Harness::drawResults(
    Rectangle bounds, bool interactive
)
{
    const bool hovered = CheckCollisionPointRec(GetMousePosition(), bounds);
    if (hovered && interactive)
    {
        resultsScroll_ -= GetMouseWheelMove() * scrollStep;
    }
    resultsScroll_ =
        std::clamp(resultsScroll_, 0.0F, std::max(0.0F, resultsHeight_ - bounds.height));

    const ui::Clip clip(bounds);
    const float top = bounds.y - resultsScroll_;
    float y = top;
    const auto next = [&y](float height) -> void { y += height + ui::spacing; };

    next(
        ui::textCard(
            "Emotions",
            std::array{ui::Paragraph{.text = emotionsText(snapshot_->emotions)}},
            {.x = bounds.x, .y = y},
            bounds.width
        )
    );
    next(
        ui::textCard(
            "Eye tracking",
            std::array{
                ui::Paragraph{.text = directionsText(snapshot_->eyeTracking)},
                ui::Paragraph{.text = ufeel::text::eyeTracking(snapshot_->eyeTracking)},
                ui::Paragraph{
                    .text =
                        "Position is relative to this window. Maximize before calibration for a "
                        "larger area.",
                    .color = ui::color::mutedText,
                },
            },
            {.x = bounds.x, .y = y},
            bounds.width
        )
    );
    next(
        ui::textCard(
            "Calibration diagnostics",
            std::array{ui::Paragraph{.text = diagnostics_}},
            {.x = bounds.x, .y = y},
            bounds.width
        )
    );
    next(drawSpeechCard({.x = bounds.x, .y = y}, bounds.width, interactive));
    y += ui::textCard(
        "Heart rate (BPM)",
        std::array{ui::Paragraph{.text = ufeel::text::heartRate(snapshot_->heartRate)}},
        {.x = bounds.x, .y = y},
        bounds.width
    );
    resultsHeight_ = y - top;
}

void Harness::drawGazeMarker() const
{
    const ufeel::EyeTrackingResult& eyeTracking = snapshot_->eyeTracking;
    if (!eyeTracking.isValid())
    {
        return;
    }

    ui::marker(
        {
            .x = eyeTracking.estimatedX * static_cast<float>(GetScreenWidth()),
            .y = eyeTracking.estimatedY * static_cast<float>(GetScreenHeight()),
        },
        markerRadius,
        ui::color::gazeMarker,
        ui::color::text
    );
}
