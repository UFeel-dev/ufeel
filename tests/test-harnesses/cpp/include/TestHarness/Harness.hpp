#ifndef TESTHARNESS_HARNESS_HPP
#define TESTHARNESS_HARNESS_HPP

#include "TestHarness/CameraFeed.hpp"
#include "TestHarness/Ui.hpp"
#include "ufeel/Calibration/CalibrationPresenter.hpp"
#include "ufeel/Calibration/EyeTrackingCalibration.hpp"
#include "ufeel/Configuration/CalibrationProfileStore.hpp"
#include "ufeel/Configuration/PackageLayout.hpp"
#include "ufeel/Model/Microphone.hpp"
#include "ufeel/Pipeline/Session.hpp"
#include "ufeel/Pipeline/Snapshot.hpp"

#include <atomic>
#include <chrono>
#include <exception>
#include <filesystem>
#include <future>
#include <memory>
#include <optional>
#include <raylib.h>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

class Harness final : public ufeel::CalibrationPresenter
{
public:

    explicit Harness(std::filesystem::path packageDirectory);
    ~Harness() override = default;

    Harness(const Harness&) = delete;
    Harness(Harness&&) = delete;
    auto operator=(const Harness&) -> Harness& = delete;
    auto operator=(Harness&&) -> Harness& = delete;

    void run();

    void show() override;
    void hide() override;
    void showTarget(float x, float y) override;
    void showInstruction(std::string_view text) override;
    void showStatus(std::string_view text) override;

private:

    void startSession(ufeel::SpeechLanguage language);
    static void showLoading(ufeel::SpeechLanguage language);

    void update();
    void draw();

    [[nodiscard]] auto status() const -> std::string_view;
    void drawCamera(Rectangle bounds) const;
    void drawResults(Rectangle bounds, bool interactive);
    void drawBottomBar(Rectangle bounds, float statusWidth, bool interactive);
    void drawGazeMarker() const;

    void toggleCalibration();
    void updateCalibration();
    void drawCalibrationOverlay() const;

    [[nodiscard]] static auto languageName(ufeel::SpeechLanguage language) -> std::string_view;
    void switchLanguage(ufeel::SpeechLanguage language);
    void refreshMicrophones();
    auto drawSpeechCard(Vector2 position, float width, bool interactive) -> float;

    template <typename Result, typename Handler>
    void poll(std::future<Result>& pending, Handler handle);

    ui::Window window_;
    std::filesystem::path packageDirectory_;
    ufeel::CalibrationProfileStore profiles_;
    std::string idleStatus_;
    CameraFeed camera_;
    std::atomic<bool> microphonesChanged_ = false;
    ufeel::SpeechLanguage language_ = ufeel::SpeechLanguage::French;
    std::optional<ufeel::SpeechLanguage> requestedLanguage_;
    std::unique_ptr<ufeel::Session> session_;
    std::unique_ptr<ufeel::EyeTrackingCalibration> calibration_;

    std::shared_ptr<const ufeel::Snapshot> snapshot_;
    std::future<void> pendingSpeech_;
    std::future<std::vector<ufeel::Microphone>> pendingMicrophones_;
    std::future<bool> pendingSelection_;
    std::string selectingName_;
    std::vector<ufeel::Microphone> microphones_;
    std::string diagnostics_ = "No calibration diagnostics yet.";
    float resultsScroll_ = 0.0F;
    float resultsHeight_ = 0.0F;

    bool calibrationVisible_ = false;
    Vector2 calibrationTarget_{.x = 0.5F, .y = 0.5F};
    std::string calibrationInstruction_;
    std::string calibrationStatus_;
};

template <typename Result, typename Handler>
void Harness::poll(
    std::future<Result>& pending, Handler handle
)
{
    if (!pending.valid() || pending.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
    {
        return;
    }

    try
    {
        if constexpr (std::is_void_v<Result>)
        {
            pending.get();
            handle();
        }
        else
        {
            handle(pending.get());
        }
    }
    catch (const std::exception& error)
    {
        idleStatus_ = error.what();
    }
}

#endif // TESTHARNESS_HARNESS_HPP
