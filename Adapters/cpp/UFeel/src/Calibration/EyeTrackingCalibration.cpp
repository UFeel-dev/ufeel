#include "ufeel/Calibration/EyeTrackingCalibration.hpp"

#include "ufeel/Calibration/CalibrationPresenter.hpp"
#include "ufeel/Calibration/CalibrationResult.hpp"
#include "ufeel/Configuration/CalibrationProfileStore.hpp"
#include "ufeel/Model/Calibration.hpp"
#include "ufeel/Model/Detection.hpp"
#include "ufeel/Model/EyeTrackingResult.hpp"
#include "ufeel/Native/Processor.hpp"
#include "ufeel/Pipeline/Clock.hpp"
#include "ufeel/Pipeline/Session.hpp"

#include <chrono>
#include <cstddef>
#include <future>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace ufeel
{

namespace
{

template <typename Result>
auto isReady(
    const std::future<Result>& future
) -> bool
{
    return future.valid() && future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

auto position(
    std::size_t index
) -> std::string
{
    return std::to_string(index + 1) + "/" + std::to_string(EyeTrackingCalibration::targets.size());
}

auto finish(
    Processor& processor, const std::optional<CalibrationProfileStore>& profiles
) -> CalibrationResult
{
    if (!processor.finishEyeTrackingCalibration())
    {
        return CalibrationResult::failure(processor.eyeTrackingCalibrationDiagnostics());
    }

    const bool saved = profiles.has_value() && profiles->save(processor);
    return CalibrationResult::success(saved, profiles.has_value());
}

} // namespace

EyeTrackingCalibration::EyeTrackingCalibration(
    Session& session,
    CalibrationPresenter& presenter,
    const CalibrationProfileStore* profiles,
    CalibrationTiming timing
)
    : session_(session)
    , presenter_(presenter)
    , profiles_(profiles)
    , timing_(timing)
{
}

EyeTrackingCalibration::~EyeTrackingCalibration()
{
    if (isRunning())
    {
        phase_ = Phase::Idle;
        releaseSession();
    }
}

void EyeTrackingCalibration::start()
{
    if (isRunning())
    {
        throw std::logic_error("Eye-tracking calibration is already running.");
    }

    pendingCommand_ = session_.execute([](Processor& processor) -> void {
        processor.beginEyeTrackingCalibration();
    });
    nativeStarted_ = true;
    session_.demand().pin(DetectionType::EyeTracking);
    session_.scheduler().setOverride(DetectionType::EyeTracking, Clock::duration::zero());

    cancelRequested_ = false;
    steadySince_.reset();
    phase_ = Phase::Beginning;
}

void EyeTrackingCalibration::cancel()
{
    cancelRequested_ = isRunning();
}

auto EyeTrackingCalibration::isRunning() const -> bool
{
    return phase_ != Phase::Idle;
}

auto EyeTrackingCalibration::update() -> std::optional<CalibrationResult>
{
    if (!isRunning())
    {
        return std::nullopt;
    }

    try
    {
        if (cancelRequested_ && phase_ != Phase::Finishing)
        {
            stop();
            return CalibrationResult::cancelled();
        }
        return advance(Clock::now());
    }
    catch (...)
    {
        if (isRunning())
        {
            stop();
        }
        throw;
    }
}

auto EyeTrackingCalibration::advance(
    Clock::time_point now
) -> std::optional<CalibrationResult>
{
    switch (phase_)
    {
    case Phase::Beginning:
        if (isReady(pendingCommand_))
        {
            pendingCommand_.get();
            presenter_.show();
            presenter_.showTarget(targets.front().x, targets.front().y);
            nextPoll_ = now;
            phase_ = Phase::WaitingForFace;
        }
        break;
    case Phase::WaitingForFace:
        waitForSteadyFace(now);
        break;
    case Phase::SettingTarget:
        if (isReady(pendingCommand_))
        {
            pendingCommand_.get();
            targetStart_ = now;
            nextPoll_ = now;
            phase_ = Phase::CollectingTarget;
        }
        break;
    case Phase::CollectingTarget:
        collectTarget(now);
        break;
    case Phase::Finishing:
        if (isReady(pendingResult_))
        {
            auto result = pendingResult_.get();
            nativeStarted_ = false;
            stop();
            return result;
        }
        break;
    case Phase::Idle:
        break;
    }

    return std::nullopt;
}

void EyeTrackingCalibration::waitForSteadyFace(
    Clock::time_point now
)
{
    if (now < nextPoll_)
    {
        return;
    }
    nextPoll_ = now + timing_.readinessPoll;

    if (session_.latest()->eyeTracking.status != EyeTrackingStatus::Calibrating)
    {
        steadySince_.reset();
        presenter_.showInstruction("Position your face in the camera view and keep your eyes open");
        presenter_.showStatus("Waiting for a valid face and eye landmarks");
        return;
    }

    if (!steadySince_)
    {
        steadySince_ = now;
    }
    presenter_.showInstruction("Face detected. Keep your face steady and eyes open");
    presenter_.showStatus("Hold a steady position before calibration");

    if (now - *steadySince_ >= timing_.steadyFace)
    {
        beginTarget(0);
    }
}

void EyeTrackingCalibration::beginTarget(
    std::size_t index
)
{
    const CalibrationTarget target = targets.at(index);
    targetIndex_ = index;

    presenter_.showTarget(target.x, target.y);
    presenter_.showInstruction(
        "Look at the target and keep your head still (" + position(index) + ")"
    );

    pendingCommand_ = session_.execute([target](Processor& processor) -> void {
        processor.setEyeTrackingCalibrationTarget(target.x, target.y);
    });
    phase_ = Phase::SettingTarget;
}

void EyeTrackingCalibration::collectTarget(
    Clock::time_point now
)
{
    if (!pendingProgress_.valid())
    {
        if (now >= nextPoll_)
        {
            pendingProgress_ = session_.execute([](Processor& processor) -> CalibrationProgress {
                return processor.eyeTrackingCalibrationProgress();
            });
        }
        return;
    }

    if (!isReady(pendingProgress_))
    {
        return;
    }

    const CalibrationProgress progress = pendingProgress_.get();
    const std::string samples = std::to_string(progress.samplesForCurrentTarget) + "/" +
                                std::to_string(progress.requiredSamplesPerTarget) +
                                " usable frames";
    presenter_.showStatus("Calibration target " + position(targetIndex_) + ": " + samples);
    presenter_.showInstruction("Keep looking at the target (" + samples + ")");

    if (!progress.currentTargetComplete() && now - targetStart_ < timing_.maxPerTarget)
    {
        nextPoll_ = now + timing_.progressPoll;
        return;
    }

    if (targetIndex_ + 1 < targets.size())
    {
        beginTarget(targetIndex_ + 1);
        return;
    }

    pendingResult_ = session_.execute(
        [profiles = profiles_ != nullptr ? std::optional(*profiles_) : std::nullopt](
            Processor& processor
        ) -> CalibrationResult { return finish(processor, profiles); }
    );
    phase_ = Phase::Finishing;
}

void EyeTrackingCalibration::releaseSession()
{
    session_.scheduler().clearOverride(DetectionType::EyeTracking);
    session_.demand().unpin(DetectionType::EyeTracking);

    pendingCommand_ = {};
    pendingProgress_ = {};
    pendingResult_ = {};

    if (std::exchange(nativeStarted_, false))
    {
        session_.execute([](Processor& processor) -> void {
            processor.cancelEyeTrackingCalibration();
        });
    }
}

void EyeTrackingCalibration::stop()
{
    phase_ = Phase::Idle;
    cancelRequested_ = false;
    releaseSession();
    presenter_.hide();
}

} // namespace ufeel
