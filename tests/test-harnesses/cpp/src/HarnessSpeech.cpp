#include "TestHarness/Harness.hpp"
#include "TestHarness/Ui.hpp"
#include "ufeel/Configuration/PackageLayout.hpp"
#include "ufeel/Model/Microphone.hpp"
#include "ufeel/Text/Text.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <iterator>
#include <raylib.h>
#include <string>
#include <string_view>

namespace
{

constexpr float rowHeight = 32.0F;
constexpr float controlHeight = 32.0F;
constexpr float refreshPadding = 24.0F;
constexpr float minimumSpeechHeight = 48.0F;

constexpr std::array languages{
    ufeel::SpeechLanguage::French,
    ufeel::SpeechLanguage::English,
    ufeel::SpeechLanguage::German,
};
constexpr std::array<std::string_view, languages.size()> languageNames{
    "French",
    "English",
    "German",
};

auto languageIndex(
    ufeel::SpeechLanguage language
) -> std::size_t
{
    return static_cast<std::size_t>(
        std::ranges::distance(languages.begin(), std::ranges::find(languages, language))
    );
}

} // namespace

auto Harness::languageName(
    ufeel::SpeechLanguage language
) -> std::string_view
{
    return languageNames.at(languageIndex(language));
}

void Harness::switchLanguage(
    ufeel::SpeechLanguage language
)
{
    const bool speechEnabled = session_->isSpeechToTextEnabled();
    const std::string microphoneId = session_->selectedMicrophoneId();
    const ufeel::SpeechLanguage previous = language_;

    pendingSpeech_ = {};
    pendingMicrophones_ = {};
    pendingSelection_ = {};
    calibration_.reset();
    session_.reset();

    try
    {
        startSession(language);
    }
    catch (const std::exception& error)
    {
        startSession(previous);
        idleStatus_ = "Could not load the " + std::string(languageName(language)) +
                      " speech model: " + error.what();
    }

    if (!microphoneId.empty())
    {
        session_->selectMicrophone(microphoneId);
    }
    if (speechEnabled)
    {
        session_->setSpeechToTextEnabled(true);
    }
}

void Harness::refreshMicrophones()
{
    if (!pendingMicrophones_.valid())
    {
        pendingMicrophones_ = session_->microphones();
    }
}

auto Harness::drawSpeechCard(
    Vector2 position, float width, bool interactive
) -> float
{
    const float innerWidth = width - (2.0F * ui::padding);
    const float left = position.x + ui::padding;
    const std::string speech = ufeel::text::speechToText(snapshot_->speechToText);
    const float speechHeight = std::max(minimumSpeechHeight, ui::textHeight(speech, innerWidth));
    const float rowsHeight = static_cast<float>(microphones_.size() + 1) * rowHeight;
    const float height = (2.0F * ui::padding) + (2.0F * ui::lineHeight) + (2.0F * controlHeight) +
                         (5.0F * ui::spacing) + rowsHeight + speechHeight;

    ui::card({.x = position.x, .y = position.y, .width = width, .height = height});
    float y = position.y + ui::padding;
    ui::title("Speech recognition", {.x = left, .y = y});
    y += ui::lineHeight + ui::spacing;

    ui::text("Language", {.x = left, .y = y}, innerWidth);
    y += ui::lineHeight + ui::spacing;
    const auto chosen = ui::segmented(
        {.x = left, .y = y, .width = innerWidth, .height = controlHeight},
        languageNames,
        languageIndex(language_),
        interactive && !calibration_->isRunning()
    );
    if (chosen && languages.at(*chosen) != language_)
    {
        requestedLanguage_ = languages.at(*chosen);
    }
    y += controlHeight + ui::spacing;

    const float refreshWidth = ui::measure("Refresh") + refreshPadding;
    ui::text(
        "Microphone", {.x = left, .y = y + ((controlHeight - ui::lineHeight) / 2.0F)}, innerWidth
    );
    if (ui::button(
            {
                .x = left + innerWidth - refreshWidth,
                .y = y,
                .width = refreshWidth,
                .height = controlHeight,
            },
            "Refresh",
            interactive && !pendingMicrophones_.valid()
        ))
    {
        refreshMicrophones();
    }
    y += controlHeight + ui::spacing;

    const std::string selectedId = session_->selectedMicrophoneId();
    const bool selectedListed = std::ranges::any_of(
        microphones_, [&selectedId](const ufeel::Microphone& microphone) -> bool {
            return microphone.id == selectedId;
        }
    );

    const auto row = [&](const std::string& id, const std::string& label) -> void {
        const bool selected = id == selectedId || (id.empty() && !selectedListed);
        const Rectangle bounds{.x = left, .y = y, .width = innerWidth, .height = rowHeight};
        if (ui::listItem(bounds, label, selected, interactive && !pendingSelection_.valid()) &&
            !selected)
        {
            selectingName_ = label;
            pendingSelection_ = session_->selectMicrophone(id);
        }
        y += rowHeight;
    };

    row({}, ufeel::text::defaultMicrophone(microphones_));
    for (const ufeel::Microphone& microphone : microphones_)
    {
        row(microphone.id, microphone.name);
    }

    ui::text(speech, {.x = left, .y = y + ui::spacing}, innerWidth);
    return height;
}
