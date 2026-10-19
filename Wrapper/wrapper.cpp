#include "DataProcessor.hpp"
#include "EyeTracking/EyeTrackingDetector.hpp"
#include "opencv2/core/hal/interface.h"
#include "HeartRate/UFeelHeartRateStatus.hpp"
#include "opencv2/core/mat.hpp"
#include "ufeel.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <string.h>
#include <string>

extern "C" {

__attribute__((visibility("default"))) auto ufeel_create(
    const char* model_directory, const char* speech_model_path
) -> void*
{
    if (model_directory == nullptr || model_directory[0] == '\0' || speech_model_path == nullptr ||
        speech_model_path[0] == '\0')
    {
        return nullptr;
    }

    try
    {
        return new DataProcessor(model_directory, speech_model_path);
    }
    catch (const std::exception& exception)
    {
        std::cerr << "[ufeel] Failed to create processor: " << exception.what() << '\n';
        return nullptr;
    }
}

__attribute__((visibility("default"))) void ufeel_destroy(
    void* processor
)
{
    delete static_cast<DataProcessor*>(processor);
}

__attribute__((visibility("default"))) auto ufeel_update(
    void* processor
) -> int32_t
{
    if (processor == nullptr)
    {
        return 0;
    }

    return static_cast<DataProcessor*>(processor)->update() ? 1 : 0;
}

__attribute__((visibility("default"))) auto ufeel_get_frame(
    void* processor
) -> UFeelFrame*
{
    if (processor == nullptr)
    {
        return nullptr;
    }

    cv::Mat const frame = static_cast<DataProcessor*>(processor)->getFrame();
    if (frame.empty() || frame.type() != CV_8UC3)
    {
        return nullptr;
    }

    auto* out = new UFeelFrame();
    out->width = static_cast<uint32_t>(frame.cols);
    out->height = static_cast<uint32_t>(frame.rows);
    out->stride = static_cast<uint32_t>(frame.cols * 3);

    size_t const size = static_cast<size_t>(out->stride) * out->height;
    out->data = new uint8_t[size];
    for (uint32_t y = 0; y < out->height; y++)
    {
        std::memcpy(out->data + (static_cast<size_t>(y) * out->stride), frame.ptr(y), out->stride);
    }
    return out;
}

__attribute__((visibility("default"))) void ufeel_free_frame(
    UFeelFrame* frame
)
{
    if (frame == nullptr)
    {
        return;
    }

    delete[] frame->data;
    delete frame;
}

__attribute__((visibility("default"))) auto ufeel_get_emotions(
    void* processor, uint32_t* size
) -> UFeelPair*
{
    if (processor == nullptr || size == nullptr)
    {
        return nullptr;
    }

    auto const& emotions = static_cast<DataProcessor*>(processor)->getEmotions();
    *size = static_cast<uint32_t>(emotions.size());
    if (*size == 0)
    {
        return nullptr;
    }

    auto* out = new UFeelPair[*size];
    uint32_t i = 0;
    for (auto const& [key, value] : emotions)
    {
        out[i].key = strdup(key.c_str());
        out[i].value = value;
        i++;
    }
    return out;
}

__attribute__((visibility("default"))) void ufeel_free_emotions(
    UFeelPair* emotions, uint32_t size
)
{
    if (emotions == nullptr)
    {
        return;
    }

    for (uint32_t i = 0; i < size; i++)
    {
        free((void*)emotions[i].key);
    }
    delete[] emotions;
}

__attribute__((visibility("default"))) auto ufeel_begin_calibration(
    void* processor
) -> int32_t
{
    if (processor == nullptr)
    {
        return UFEEL_STATUS_INVALID_ARGUMENT;
    }

    static_cast<DataProcessor*>(processor)->beginEyeTrackingCalibration();
    return UFEEL_STATUS_OK;
}

__attribute__((visibility("default"))) auto ufeel_set_calibration_target(
    void* processor, float x, float y
) -> int32_t
{
    if (processor == nullptr || !std::isfinite(x) || !std::isfinite(y) || x < 0.0F || x > 1.0F ||
        y < 0.0F || y > 1.0F)
    {
        return UFEEL_STATUS_INVALID_ARGUMENT;
    }

    if (!static_cast<DataProcessor*>(processor)->setEyeTrackingCalibrationTarget(x, y))
    {
        return UFEEL_STATUS_INVALID_STATE;
    }
    return UFEEL_STATUS_OK;
}

__attribute__((visibility("default"))) auto ufeel_finish_calibration(
    void* processor
) -> int32_t
{
    if (processor == nullptr)
    {
        return UFEEL_STATUS_INVALID_ARGUMENT;
    }

    return static_cast<DataProcessor*>(processor)->finishEyeTrackingCalibration()
               ? UFEEL_STATUS_OK
               : UFEEL_STATUS_CALIBRATION_FAILED;
}

__attribute__((visibility("default"))) auto ufeel_cancel_calibration(
    void* processor
) -> int32_t
{
    if (processor == nullptr)
    {
        return UFEEL_STATUS_INVALID_ARGUMENT;
    }

    static_cast<DataProcessor*>(processor)->cancelEyeTrackingCalibration();
    return UFEEL_STATUS_OK;
}

__attribute__((visibility("default"))) auto ufeel_get_gaze(
    void* processor, UFeelGaze* gaze
) -> int32_t
{
    if (processor == nullptr || gaze == nullptr)
    {
        return UFEEL_STATUS_INVALID_ARGUMENT;
    }

    *gaze = {};
    auto const result = static_cast<DataProcessor*>(processor)->getGaze();
    if (result.status != GazeStatus::Valid)
    {
        return UFEEL_STATUS_NO_GAZE;
    }

    gaze->x = result.x;
    gaze->y = result.y;
    gaze->directions = result.directions;
    gaze->valid = 1;
    return UFEEL_STATUS_OK;
}

__attribute__((visibility("default"))) auto ufeel_get_gaze_ex(
    void* processor, UFeelGazeEx* gaze
) -> int32_t
{
    if (processor == nullptr || gaze == nullptr)
    {
        return UFEEL_STATUS_INVALID_ARGUMENT;
    }

    *gaze = {};
    auto const result = static_cast<DataProcessor*>(processor)->getGaze();
    gaze->status = static_cast<uint32_t>(result.status);
    if (result.status != GazeStatus::Valid)
    {
        return UFEEL_STATUS_NO_GAZE;
    }

    gaze->x = result.x;
    gaze->y = result.y;
    gaze->confidence = result.confidence;
    gaze->directions = result.directions;
    return UFEEL_STATUS_OK;
}

__attribute__((visibility("default"))) auto ufeel_get_gaze_result(
    void* processor, UFeelGazeResult* gaze
) -> int32_t
{
    if (processor == nullptr || gaze == nullptr)
    {
        return UFEEL_STATUS_INVALID_ARGUMENT;
    }

    *gaze = {};
    auto const result = static_cast<DataProcessor*>(processor)->getGaze();
    gaze->status = static_cast<uint32_t>(result.status);

    if (result.status != GazeStatus::Valid)
    {
        return UFEEL_STATUS_NO_GAZE;
    }

    gaze->recorded_x = result.recordedX;
    gaze->recorded_y = result.recordedY;
    gaze->estimated_x = result.x;
    gaze->estimated_y = result.y;
    gaze->confidence = result.confidence;
    gaze->directions = result.directions;

    return UFEEL_STATUS_OK;
}

__attribute__((visibility("default"))) auto ufeel_get_calibration_progress(
    void* processor, UFeelCalibrationProgress* progress
) -> int32_t
{
    if (processor == nullptr || progress == nullptr)
    {
        return UFEEL_STATUS_INVALID_ARGUMENT;
    }

    *progress = {};
    auto const result = static_cast<DataProcessor*>(processor)->getEyeTrackingCalibrationProgress();
    progress->targets_with_enough_samples = result.targetsWithEnoughSamples;
    progress->target_count = result.targetCount;
    progress->samples_for_current_target = result.samplesForCurrentTarget;
    progress->required_samples_per_target = result.requiredSamplesPerTarget;
    return UFEEL_STATUS_OK;
}

__attribute__((visibility("default"))) auto ufeel_get_calibration_diagnostics(void* processor, UFeelCalibrationDiagnostics* diagnostics) -> int32_t
{
    if (processor == nullptr || diagnostics == nullptr)
    {
        return UFEEL_STATUS_INVALID_ARGUMENT;
    }

    *diagnostics = {};
    auto const result = static_cast<DataProcessor*>(processor)->getEyeTrackingCalibrationDiagnostics();
    diagnostics->failure_reason = static_cast<int32_t>(result.failureReason);
    diagnostics->input_target_count = result.inputTargetCount;
    diagnostics->valid_target_count = result.validTargetCount;
    diagnostics->targets_with_enough_raw_samples = result.targetsWithEnoughRawSamples;
    diagnostics->targets_with_enough_retained_samples = result.targetsWithEnoughRetainedSamples;
    diagnostics->minimum_retained_samples = result.minimumRetainedSamples;
    diagnostics->maximum_retained_samples = result.maximumRetainedSamples;
    diagnostics->horizontal_span = result.horizontalSpan;
    diagnostics->vertical_span = result.verticalSpan;
    diagnostics->mean_cross_validation_error = result.meanCrossValidationError;
    diagnostics->maximum_cross_validation_error = result.maximumCrossValidationError;
    diagnostics->cross_validation_targets_evaluated = result.crossValidationTargetsEvaluated;
    diagnostics->regression_failures = result.regressionFailures;
    diagnostics->invalid_target_count = result.invalidTargetCount;
    diagnostics->targets_with_insufficient_raw_samples = result.targetsWithInsufficientRawSamples;
    diagnostics->targets_with_insufficient_retained_samples = result.targetsWithInsufficientRetainedSamples;
    diagnostics->mean_error_exceeded = result.meanErrorExceeded ? 1 : 0;
    diagnostics->maximum_error_exceeded = result.maximumErrorExceeded ? 1 : 0;
    return UFEEL_STATUS_OK;
}

__attribute__((visibility("default"))) auto ufeel_save_calibration_profile(
    void* processor, const char* profile_path
) -> int32_t
{
    if (processor == nullptr || profile_path == nullptr || profile_path[0] == '\0')
    {
        return UFEEL_STATUS_INVALID_ARGUMENT;
    }

    try
    {
        return static_cast<DataProcessor*>(processor)->saveEyeTrackingCalibrationProfile(
                   profile_path
               )
                   ? UFEEL_STATUS_OK
                   : UFEEL_STATUS_PROFILE_FAILED;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "[ufeel] Failed to save eye-tracking profile: " << exception.what() << '\n';
        return UFEEL_STATUS_PROFILE_FAILED;
    }
}

__attribute__((visibility("default"))) auto ufeel_load_calibration_profile(
    void* processor, const char* profile_path
) -> int32_t
{
    if (processor == nullptr || profile_path == nullptr || profile_path[0] == '\0')
    {
        return UFEEL_STATUS_INVALID_ARGUMENT;
    }

    try
    {
        return static_cast<DataProcessor*>(processor)->loadEyeTrackingCalibrationProfile(
                   profile_path
               )
                   ? UFEEL_STATUS_OK
                   : UFEEL_STATUS_PROFILE_FAILED;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "[ufeel] Failed to load eye-tracking profile: " << exception.what() << '\n';
        return UFEEL_STATUS_PROFILE_FAILED;
    }
}

__attribute__((visibility("default"))) void ufeel_toggle_speech(
    void* processor, uint8_t state
)
{
    if (processor == nullptr)
    {
        return;
    }

    static_cast<DataProcessor*>(processor)->toggleSpeechToText(state != 0);
}

__attribute__((visibility("default"))) auto ufeel_get_speech(
    void* processor
) -> char*
{
    if (processor == nullptr)
    {
        return nullptr;
    }

    std::string const& speech = static_cast<DataProcessor*>(processor)->getSpeech();
    return strdup(speech.c_str());
}

__attribute__((visibility("default"))) void ufeel_free_speech(
    char* speech
)
{
    free(speech);
}
}

__attribute__((visibility("default"))) auto ufeel_get_heartrate(
    void* processor, UFeelHeartRateResult* result
) -> bool
{
    if (processor == nullptr || result == nullptr)
    {
        return false;
    } // TODO(augustin-grosnon): update this method to use same logic as eye tracking

    *result = {};

    const auto heartrate = static_cast<DataProcessor*>(processor)->getHeartRate();

    result->bpm = heartrate.bpm;
    result->status = static_cast<int32_t>(heartrate.status);

    return true;
}
