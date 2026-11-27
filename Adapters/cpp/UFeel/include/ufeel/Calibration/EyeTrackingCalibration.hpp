#ifndef UFEEL_CALIBRATION_EYETRACKINGCALIBRATION_HPP
#define UFEEL_CALIBRATION_EYETRACKINGCALIBRATION_HPP

#include "ufeel/Calibration/CalibrationPresenter.hpp"
#include "ufeel/Calibration/CalibrationResult.hpp"
#include "ufeel/Configuration/CalibrationProfileStore.hpp"
#include "ufeel/Model/Calibration.hpp"
#include "ufeel/Pipeline/Clock.hpp"
#include "ufeel/Pipeline/Session.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <future>
#include <optional>

namespace ufeel
{

struct CalibrationTarget
{
    float x = 0.0F;
    float y = 0.0F;
};

struct CalibrationTiming
{
    Clock::duration steadyFace = std::chrono::seconds(2);
    Clock::duration readinessPoll = std::chrono::milliseconds(100);
    Clock::duration maxPerTarget = std::chrono::seconds(4);
    Clock::duration progressPoll = std::chrono::milliseconds(200);
};

class EyeTrackingCalibration
{
public:

    static constexpr std::array<CalibrationTarget, 9> targets{
        {
            {.x = 0.5F, .y = 0.5F},
            {.x = 0.1F, .y = 0.1F},
            {.x = 0.1F, .y = 0.9F},
            {.x = 0.9F, .y = 0.1F},
            {.x = 0.9F, .y = 0.9F},
            {.x = 0.1F, .y = 0.5F},
            {.x = 0.5F, .y = 0.1F},
            {.x = 0.9F, .y = 0.5F},
            {.x = 0.5F, .y = 0.9F},
        },
    };

    EyeTrackingCalibration(
        Session& session,
        CalibrationPresenter& presenter,
        const CalibrationProfileStore* profiles = nullptr,
        CalibrationTiming timing = {}
    );
    ~EyeTrackingCalibration();

    EyeTrackingCalibration(const EyeTrackingCalibration&) = delete;
    auto operator=(const EyeTrackingCalibration&) -> EyeTrackingCalibration& = delete;
    EyeTrackingCalibration(EyeTrackingCalibration&&) = delete;
    auto operator=(EyeTrackingCalibration&&) -> EyeTrackingCalibration& = delete;

    void start();
    void cancel();

    [[nodiscard]] auto isRunning() const -> bool;

    auto update() -> std::optional<CalibrationResult>;

private:

    enum class Phase : std::uint8_t
    {
        Idle,
        Beginning,
        WaitingForFace,
        SettingTarget,
        CollectingTarget,
        Finishing,
    };

    [[nodiscard]] auto advance(Clock::time_point now) -> std::optional<CalibrationResult>;
    void waitForSteadyFace(Clock::time_point now);
    void beginTarget(std::size_t index);
    void collectTarget(Clock::time_point now);

    void releaseSession();
    void stop();

    Session& session_;
    CalibrationPresenter& presenter_;
    const CalibrationProfileStore* profiles_;
    CalibrationTiming timing_;

    Phase phase_ = Phase::Idle;
    bool cancelRequested_ = false;
    bool nativeStarted_ = false;
    std::size_t targetIndex_ = 0;
    std::optional<Clock::time_point> steadySince_;
    Clock::time_point nextPoll_;
    Clock::time_point targetStart_;

    std::future<void> pendingCommand_;
    std::future<CalibrationProgress> pendingProgress_;
    std::future<CalibrationResult> pendingResult_;
};

} // namespace ufeel

#endif // UFEEL_CALIBRATION_EYETRACKINGCALIBRATION_HPP
