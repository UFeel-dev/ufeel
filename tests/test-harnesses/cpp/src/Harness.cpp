#include "TestHarness/Harness.hpp"

#include "TestHarness/Ui.hpp"
#include "ufeel/Calibration/EyeTrackingCalibration.hpp"
#include "ufeel/Configuration/CalibrationProfileStore.hpp"
#include "ufeel/Configuration/PackageLayout.hpp"
#include "ufeel/Model/Detection.hpp"
#include "ufeel/Model/Flags.hpp"
#include "ufeel/Model/Microphone.hpp"
#include "ufeel/Native/Processor.hpp"
#include "ufeel/Pipeline/Session.hpp"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <memory>
#include <optional>
#include <raylib.h>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{

constexpr int initialWidth = 1280;
constexpr int initialHeight = 850;
constexpr float resultsWidth = 380.0F;
constexpr float buttonHeight = 40.0F;
constexpr float buttonPadding = 32.0F;
constexpr float minimumBarHeight = 72.0F;
constexpr float loadingSize = 30.0F;

auto loadProfile(
    const ufeel::CalibrationProfileStore& profiles, ufeel::Processor& processor
) -> std::string
{
    try
    {
        switch (profiles.load(processor))
        {
        case ufeel::ProfileLoadResult::Loaded:
            return "Loaded saved eye-tracking calibration";
        case ufeel::ProfileLoadResult::Invalid:
            return "Saved eye-tracking calibration is invalid, recalibrate to replace it";
        case ufeel::ProfileLoadResult::Missing:
            break;
        }
    }
    catch (const std::exception& error)
    {
        return std::string("Could not load eye-tracking calibration: ") + error.what();
    }
    return {};
}

auto widestLabel(
    std::string_view first, std::string_view second
) -> float
{
    return std::max(ui::measure(first), ui::measure(second)) + buttonPadding;
}

} // namespace

Harness::Harness(
    std::filesystem::path packageDirectory
)
    : window_(initialWidth, initialHeight, "UFeel Integration Test (C++)")
    , packageDirectory_(std::move(packageDirectory))
    , profiles_(ufeel::CalibrationProfileStore::createDefault())
{
    startSession(language_);
}

void Harness::startSession(
    ufeel::SpeechLanguage language
)
{
    showLoading(language);
    ufeel::Processor processor(ufeel::PackageLayout::fromRoot(packageDirectory_, language));
    idleStatus_ = loadProfile(profiles_, processor);

    session_ = std::make_unique<ufeel::Session>(
        std::move(processor),
        ufeel::SessionOptions{
            .onFrameCaptured = [this](ufeel::Processor& frameSource) -> void {
                camera_.capture(frameSource);
            },
            .onMicrophonesChanged = [this]() -> void { microphonesChanged_ = true; },
        }
    );
    calibration_ = std::make_unique<ufeel::EyeTrackingCalibration>(*session_, *this, &profiles_);
    snapshot_ = session_->latest();
    language_ = language;

    session_->demand().pin(ufeel::allDetectionTypes & ~ufeel::DetectionType::SpeechToText);
    session_->start();
    refreshMicrophones();
}

void Harness::showLoading(
    ufeel::SpeechLanguage language
)
{
    BeginDrawing();
    ClearBackground(ui::color::background);
    ui::centeredText(
        "Loading models (speech: " + std::string(languageName(language)) + ")...",
        (static_cast<float>(GetScreenHeight()) - loadingSize) / 2.0F,
        loadingSize,
        ui::color::text
    );
    EndDrawing();
}

void Harness::run()
{
    while (!WindowShouldClose())
    {
        update();
        BeginDrawing();
        draw();
        EndDrawing();
    }
}

void Harness::update()
{
    if (const auto language = std::exchange(requestedLanguage_, std::nullopt))
    {
        switchLanguage(*language);
    }

    snapshot_ = session_->latest();
    camera_.upload();

    for (int key = GetKeyPressed(); key != KEY_NULL; key = GetKeyPressed())
    {
        if (key == KEY_ESCAPE)
        {
            calibration_->cancel();
        }
    }

    if (microphonesChanged_.exchange(false))
    {
        refreshMicrophones();
    }

    poll(pendingSpeech_, []() -> void {});
    poll(pendingMicrophones_, [this](std::vector<ufeel::Microphone> microphones) -> void {
        microphones_ = std::move(microphones);
    });
    poll(pendingSelection_, [this](bool selected) -> void {
        if (!selected)
        {
            idleStatus_ = "Microphone \"" + selectingName_ + "\" is no longer available";
            refreshMicrophones();
        }
    });
    updateCalibration();
}

void Harness::draw()
{
    ClearBackground(ui::color::background);

    const auto width = static_cast<float>(GetScreenWidth());
    const auto height = static_cast<float>(GetScreenHeight());
    const bool interactive = !calibrationVisible_;

    const float buttonsWidth = widestLabel("Enable speech", "Disable speech") +
                               widestLabel("Calibrate / recalibrate", "Cancel calibration") +
                               ui::spacing;
    const float statusWidth = width - (4.0F * ui::padding) - buttonsWidth - ui::spacing;
    const float barHeight =
        std::max(minimumBarHeight, ui::textHeight(status(), statusWidth) + (2.0F * ui::padding));

    const Rectangle bar{
        .x = ui::padding,
        .y = height - ui::padding - barHeight,
        .width = width - (2.0F * ui::padding),
        .height = barHeight,
    };
    const float contentHeight = bar.y - (2.0F * ui::padding);
    const Rectangle results{
        .x = width - ui::padding - resultsWidth,
        .y = ui::padding,
        .width = resultsWidth,
        .height = contentHeight,
    };
    const Rectangle camera{
        .x = ui::padding,
        .y = ui::padding,
        .width = results.x - (2.0F * ui::padding),
        .height = contentHeight,
    };

    drawCamera(camera);
    drawResults(results, interactive);
    drawBottomBar(bar, statusWidth, interactive);

    if (calibrationVisible_)
    {
        drawCalibrationOverlay();
    }
    else
    {
        drawGazeMarker();
    }
}

auto Harness::status() const -> std::string_view
{
    if (calibrationVisible_)
    {
        return calibrationStatus_;
    }
    return snapshot_->message.empty() ? idleStatus_ : snapshot_->message;
}

void Harness::drawCamera(
    Rectangle bounds
) const
{
    constexpr float inset = 8.0F;

    ui::card(bounds, ui::color::cameraCard);
    ui::title(
        "Camera", {.x = bounds.x + ui::padding, .y = bounds.y + ui::padding}, ui::color::lightText
    );

    const float top = bounds.y + ui::padding + ui::lineHeight + ui::spacing;
    const Rectangle view{
        .x = bounds.x + inset,
        .y = top,
        .width = bounds.width - (2.0F * inset),
        .height = bounds.y + bounds.height - inset - top,
    };
    ui::card(view, ui::color::cameraView);
    camera_.draw(view);
}

void Harness::drawBottomBar(
    Rectangle bounds, float statusWidth, bool interactive
)
{
    ui::card(bounds);

    const float buttonY = bounds.y + ((bounds.height - buttonHeight) / 2.0F);
    const float calibrateWidth = widestLabel("Calibrate / recalibrate", "Cancel calibration");
    const float speechWidth = widestLabel("Enable speech", "Disable speech");
    const Rectangle calibrate{
        .x = bounds.x + bounds.width - ui::padding - calibrateWidth,
        .y = buttonY,
        .width = calibrateWidth,
        .height = buttonHeight,
    };
    const Rectangle speech{
        .x = calibrate.x - ui::spacing - speechWidth,
        .y = buttonY,
        .width = speechWidth,
        .height = buttonHeight,
    };

    const std::string_view text = status();
    ui::text(
        text,
        {
            .x = bounds.x + ui::padding,
            .y = bounds.y + ((bounds.height - ui::textHeight(text, statusWidth)) / 2.0F),
        },
        statusWidth
    );

    const bool speechEnabled = session_->isSpeechToTextEnabled();
    if (ui::button(
            speech,
            speechEnabled ? "Disable speech" : "Enable speech",
            interactive && !pendingSpeech_.valid()
        ))
    {
        pendingSpeech_ = session_->setSpeechToTextEnabled(!speechEnabled);
    }

    if (ui::button(
            calibrate,
            calibration_->isRunning() ? "Cancel calibration" : "Calibrate / recalibrate",
            interactive
        ))
    {
        toggleCalibration();
    }
}
