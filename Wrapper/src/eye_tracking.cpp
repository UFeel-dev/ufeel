#include "DataProcessor.hpp"
#include "EyeTracking/EyeTrackingDetector.hpp"
#include "internal/ErrorHandling.hpp"
#include "internal/FrameConversion.hpp"
#include "internal/UFeelProcessor.hpp" // IWYU pragma: keep
#include "ufeel.h"

#include <cmath>
#include <cstdint>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>

using ufeel::internal::convertFrame;
using ufeel::internal::FrameCopyPolicy;
using ufeel::internal::guarded;
using ufeel::internal::guardedVoid;
using ufeel::internal::toUFeelStatus;

extern "C" {

auto ufeel_update_eye_tracking(
    UFeelProcessor* processor
) -> int32_t
{
    return guarded(
        "ufeel_update_eye_tracking", int32_t{UFEEL_STATUS_INTERNAL_ERROR}, [processor] -> int32_t {
            if (processor == nullptr)
            {
                return UFEEL_STATUS_INVALID_PROCESSOR;
            }
            const auto updateStatus = processor->processor->updateEyeTracking();
            if (updateStatus != DataProcessorStatus::Ok)
            {
                return toUFeelStatus(updateStatus);
            }

            return processor->processor->getEyeTracking().status == EyeTrackingStatus::Valid
                       ? UFEEL_STATUS_OK
                       : UFEEL_STATUS_NO_EYE_TRACKING;
        }
    );
}

auto ufeel_update_eye_tracking_from_frame(
    UFeelProcessor* processor, const UFeelFrame* frame
) -> int32_t
{
    return guarded(
        "ufeel_update_eye_tracking_from_frame",
        int32_t{UFEEL_STATUS_INTERNAL_ERROR},
        [processor, frame] -> int32_t {
            if (processor == nullptr)
            {
                return UFEEL_STATUS_INVALID_PROCESSOR;
            }

            const cv::Mat frameConverted = convertFrame(frame, FrameCopyPolicy::Borrow);

            if (frameConverted.empty())
            {
                return UFEEL_STATUS_INVALID_FRAME;
            }

            const auto updateStatus =
                processor->processor->updateEyeTrackingFromFrame(frameConverted);
            if (updateStatus != DataProcessorStatus::Ok)
            {
                return toUFeelStatus(updateStatus);
            }

            return processor->processor->getEyeTracking().status == EyeTrackingStatus::Valid
                       ? UFEEL_STATUS_OK
                       : UFEEL_STATUS_NO_EYE_TRACKING;
        }
    );
}

auto ufeel_begin_eye_tracking_calibration(
    UFeelProcessor* processor
) -> int32_t
{
    return guarded(
        "ufeel_begin_eye_tracking_calibration",
        int32_t{UFEEL_STATUS_INTERNAL_ERROR},
        [processor] -> UFeelStatus {
            if (processor == nullptr)
            {
                return UFEEL_STATUS_INVALID_PROCESSOR;
            }
            processor->processor->beginEyeTrackingCalibration();
            return UFEEL_STATUS_OK;
        }
    );
}

auto ufeel_set_eye_tracking_calibration_target(
    UFeelProcessor* processor, float screenX, float screenY
) -> int32_t
{
    return guarded(
        "ufeel_set_eye_tracking_calibration_target",
        int32_t{UFEEL_STATUS_INTERNAL_ERROR},
        [processor, screenX, screenY] -> UFeelStatus {
            if (processor == nullptr)
            {
                return UFEEL_STATUS_INVALID_PROCESSOR;
            }

            if (!std::isfinite(screenX) || !std::isfinite(screenY))
            {
                return UFEEL_STATUS_INVALID_ARGUMENT;
            }
            return processor->processor->setEyeTrackingCalibrationTarget(screenX, screenY)
                       ? UFEEL_STATUS_OK
                       : UFEEL_STATUS_INVALID_STATE;
        }
    );
}

auto ufeel_finish_eye_tracking_calibration(
    UFeelProcessor* processor
) -> int32_t
{
    return guarded(
        "ufeel_finish_eye_tracking_calibration",
        int32_t{UFEEL_STATUS_INTERNAL_ERROR},
        [processor] -> UFeelStatus {
            if (processor == nullptr)
            {
                return UFEEL_STATUS_INVALID_PROCESSOR;
            }
            return processor->processor->finishEyeTrackingCalibration()
                       ? UFEEL_STATUS_OK
                       : UFEEL_STATUS_EYE_TRACKING_CALIBRATION_FAILED;
        }
    );
}

auto ufeel_cancel_eye_tracking_calibration(
    UFeelProcessor* processor
) -> int32_t
{
    return guarded(
        "ufeel_cancel_eye_tracking_calibration",
        int32_t{UFEEL_STATUS_INTERNAL_ERROR},
        [processor] -> UFeelStatus {
            if (processor == nullptr)
            {
                return UFEEL_STATUS_INVALID_PROCESSOR;
            }
            processor->processor->cancelEyeTrackingCalibration();
            return UFEEL_STATUS_OK;
        }
    );
}

auto ufeel_get_eye_tracking_result(
    UFeelProcessor* processor, UFeelEyeTrackingResult* result
) -> int32_t
{
    return guarded(
        "ufeel_get_eye_tracking_result",
        int32_t{UFEEL_STATUS_INTERNAL_ERROR},
        [processor, result] -> UFeelStatus {
            if (processor == nullptr)
            {
                return UFEEL_STATUS_INVALID_PROCESSOR;
            }
            if (result == nullptr)
            {
                return UFEEL_STATUS_INVALID_ARGUMENT;
            }

            *result = {};

            const auto eyeTracking = processor->processor->getEyeTracking();

            result->status = static_cast<int32_t>(eyeTracking.status);
            result->x = static_cast<float>(eyeTracking.x);
            result->y = static_cast<float>(eyeTracking.y);
            result->recordedX = static_cast<float>(eyeTracking.recordedX);
            result->recordedY = static_cast<float>(eyeTracking.recordedY);
            result->confidence = static_cast<float>(eyeTracking.confidence);
            result->directions = static_cast<int32_t>(eyeTracking.directions);

            return eyeTracking.status == EyeTrackingStatus::Valid ? UFEEL_STATUS_OK
                                                                  : UFEEL_STATUS_NO_EYE_TRACKING;
        }
    );
}

void ufeel_get_eye_tracking_result_from_frame(
    UFeelProcessor* processor, UFeelFrame* frame, void (*handler)(UFeelEyeTrackingResult* gaze)
)
{
    guardedVoid("ufeel_get_eye_tracking_result_from_frame", [processor, frame, handler] -> void {
        if (handler == nullptr)
        {
            throw std::invalid_argument("eye-tracking result handler must not be null");
        }

        auto result = std::make_unique<UFeelEyeTrackingResult>();

        *result = {};

        if (processor == nullptr)
        {
            result->status = UFEEL_GAZE_STATUS_INVALID_PROCESSOR;
            handler(result.get());
            return;
        }

        const cv::Mat frameConverted = convertFrame(frame, FrameCopyPolicy::Borrow);

        if (frameConverted.empty())
        {
            result->status = UFEEL_GAZE_STATUS_FRAME_UNAVAILABLE;
            handler(result.get());
            return;
        }

        const auto gaze = processor->processor->getEyeTrackingFromFrame(frameConverted);

        result->status = static_cast<int32_t>(gaze.status);
        result->x = static_cast<float>(gaze.x);
        result->y = static_cast<float>(gaze.y);
        result->recordedX = static_cast<float>(gaze.recordedX);
        result->recordedY = static_cast<float>(gaze.recordedY);
        result->confidence = static_cast<float>(gaze.confidence);
        result->directions = static_cast<int32_t>(gaze.directions);

        handler(result.get());
    });
}

auto ufeel_get_eye_tracking_calibration_progress(
    UFeelProcessor* processor, UFeelEyeTrackingCalibrationProgress* progress
) -> int32_t
{
    return guarded(
        "ufeel_get_eye_tracking_calibration_progress",
        int32_t{UFEEL_STATUS_INTERNAL_ERROR},
        [processor, progress] -> UFeelStatus {
            if (processor == nullptr)
            {
                return UFEEL_STATUS_INVALID_PROCESSOR;
            }
            if (progress == nullptr)
            {
                return UFEEL_STATUS_INVALID_ARGUMENT;
            }

            *progress = {};

            const auto value = processor->processor->getEyeTrackingCalibrationProgress();

            progress->targetsWithEnoughSamples =
                static_cast<int32_t>(value.targetsWithEnoughSamples);
            progress->targetCount = static_cast<int32_t>(value.targetCount);
            progress->samplesForCurrentTarget = static_cast<int32_t>(value.samplesForCurrentTarget);
            progress->requiredSamplesPerTarget =
                static_cast<int32_t>(value.requiredSamplesPerTarget);

            return UFEEL_STATUS_OK;
        }
    );
}

auto ufeel_get_eye_tracking_calibration_diagnostics(
    UFeelProcessor* processor, UFeelEyeTrackingCalibrationDiagnostics* diagnostics
) -> int32_t
{
    return guarded(
        "ufeel_get_eye_tracking_calibration_diagnostics",
        int32_t{UFEEL_STATUS_INTERNAL_ERROR},
        [processor, diagnostics] -> UFeelStatus {
            if (processor == nullptr)
            {
                return UFEEL_STATUS_INVALID_PROCESSOR;
            }
            if (diagnostics == nullptr)
            {
                return UFEEL_STATUS_INVALID_ARGUMENT;
            }

            *diagnostics = {};
            const auto value = processor->processor->getEyeTrackingCalibrationDiagnostics();

            diagnostics->failureReason = static_cast<int32_t>(value.failureReason);

            diagnostics->inputTargetCount = static_cast<int32_t>(value.inputTargetCount);
            diagnostics->targetsWithEnoughRawSamples =
                static_cast<int32_t>(value.targetsWithEnoughRawSamples);
            diagnostics->targetsWithEnoughRetainedSamples =
                static_cast<int32_t>(value.targetsWithEnoughRetainedSamples);
            diagnostics->validTargetCount = static_cast<int32_t>(value.validTargetCount);

            diagnostics->minimumRetainedSamples =
                static_cast<int32_t>(value.minimumRetainedSamples);
            diagnostics->maximumRetainedSamples =
                static_cast<int32_t>(value.maximumRetainedSamples);

            diagnostics->horizontalSpan = static_cast<float>(value.horizontalSpan);
            diagnostics->verticalSpan = static_cast<float>(value.verticalSpan);

            diagnostics->meanCrossValidationError =
                static_cast<float>(value.meanCrossValidationError);
            diagnostics->maximumCrossValidationError =
                static_cast<float>(value.maximumCrossValidationError);
            diagnostics->crossValidationTargetsEvaluated =
                static_cast<int32_t>(value.crossValidationTargetsEvaluated);

            diagnostics->meanErrorExceeded = value.meanErrorExceeded ? 1 : 0;
            diagnostics->maximumErrorExceeded = value.maximumErrorExceeded ? 1 : 0;

            diagnostics->regressionFailures = static_cast<int32_t>(value.regressionFailures);
            diagnostics->invalidTargetCount = static_cast<int32_t>(value.invalidTargetCount);
            diagnostics->targetsWithInsufficientRawSamples =
                static_cast<int32_t>(value.targetsWithInsufficientRawSamples);
            diagnostics->targetsWithInsufficientRetainedSamples =
                static_cast<int32_t>(value.targetsWithInsufficientRetainedSamples);

            return UFEEL_STATUS_OK;
        }
    );
}

auto ufeel_save_eye_tracking_calibration_profile(
    UFeelProcessor* processor, const char* utf8Path
) -> int32_t
{
    return guarded(
        "ufeel_save_eye_tracking_calibration_profile",
        int32_t{UFEEL_STATUS_INTERNAL_ERROR},
        [processor, utf8Path] -> UFeelStatus {
            if (processor == nullptr)
            {
                return UFEEL_STATUS_INVALID_PROCESSOR;
            }
            if (utf8Path == nullptr || utf8Path[0] == '\0')
            {
                return UFEEL_STATUS_INVALID_ARGUMENT;
            }
            return processor->processor->saveEyeTrackingCalibrationProfile(utf8Path)
                       ? UFEEL_STATUS_OK
                       : UFEEL_STATUS_EYE_TRACKING_PROFILE_FAILED;
        }
    );
}

auto ufeel_load_eye_tracking_calibration_profile(
    UFeelProcessor* processor, const char* utf8Path
) -> int32_t
{
    return guarded(
        "ufeel_load_eye_tracking_calibration_profile",
        int32_t{UFEEL_STATUS_INTERNAL_ERROR},
        [processor, utf8Path] -> UFeelStatus {
            if (processor == nullptr)
            {
                return UFEEL_STATUS_INVALID_PROCESSOR;
            }
            if (utf8Path == nullptr || utf8Path[0] == '\0')
            {
                return UFEEL_STATUS_INVALID_ARGUMENT;
            }
            return processor->processor->loadEyeTrackingCalibrationProfile(utf8Path)
                       ? UFEEL_STATUS_OK
                       : UFEEL_STATUS_EYE_TRACKING_PROFILE_FAILED;
        }
    );
}

auto ufeel_create_eye_tracking_result() -> UFeelEyeTrackingResult*
{
    return guarded(
        "ufeel_create_eye_tracking_result",
        static_cast<UFeelEyeTrackingResult*>(nullptr),
        [] -> UFeelEyeTrackingResult* {
            auto* result = new UFeelEyeTrackingResult{};

            *result = {};
            return result;
        }
    );
}

void ufeel_free_eye_tracking_result(
    UFeelEyeTrackingResult* eyetracking
)
{
    guardedVoid("ufeel_free_eye_tracking_result", [eyetracking] -> void {
        if (eyetracking == nullptr)
        {
            return;
        }

        delete eyetracking;
    });
}
}
