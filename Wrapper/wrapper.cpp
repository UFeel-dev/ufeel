#include "DataProcessor.hpp"
#include "EyeTracking/EyeTrackingDetector.hpp"
#include "opencv2/core/mat.hpp"
#include "ufeel.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <limits>
#include <memory>
#include <new>
#include <string>
#include <utility>

struct UFeelProcessor
{
    DataProcessor processor;

    UFeelProcessor(
        const std::string& modelDirectory, const std::string& speechModelPath
    )
        : processor(modelDirectory, speechModelPath)
    {
    }
};

namespace
{

template <typename Result, typename Function>
auto guarded(
    const char* operation, Result fallback, Function&& function
) noexcept -> Result
{
    try
    {
        return std::forward<Function>(function)();
    }
    catch (const std::exception& exception)
    {
        std::cerr << "UFeel: " << operation << " failed: " << exception.what() << '\n';
    }
    catch (const std::exception& exception)
    {
        std::cerr << "UFeel: " << operation << " failed with an unknown exception\n";
    }

    return fallback;
}

template <typename Function>
void guardedVoid(
    const char* operation, Function&& function
) noexcept
{
    try
    {
        std::forward<Function>(function)();
    }
    catch (const std::exception& exception)
    {
        std::cerr << "UFeel: " << operation << " failed: " << exception.what() << '\n';
    }
    catch (...)
    {
        std::cerr << "UFeel: " << operation << " failed with an unknown exception\n";
    }
}

auto isValidProcessor(
    const UFeelProcessor* processor
) -> bool
{
    return processor != nullptr;
}

auto duplicateUtf8(
    const std::string& value
) -> char*
{
    auto* result = static_cast<char*>(std::malloc(value.size() + 1));
    if (result == nullptr)
    {
        return nullptr;
    }

    std::memcpy(result, value.c_str(), value.size() + 1);
    return result;
}

} // namespace

extern "C" {
auto ufeel_create(
    const char* utf8ModelDirectory, const char* utf8SpeechModelPath
) -> UFeelProcessor*
{
    if (utf8ModelDirectory == nullptr || utf8ModelDirectory[0] == '\0' ||
        utf8SpeechModelPath == nullptr || utf8SpeechModelPath[0] == '\0')
    {
        return nullptr;
    }

    return guarded(
        "ufeel_create",
        static_cast<UFeelProcessor*>(nullptr),
        [utf8ModelDirectory, utf8SpeechModelPath] -> UFeelProcessor* {
            return new (std::nothrow) UFeelProcessor(utf8ModelDirectory, utf8SpeechModelPath);
        }
    );
}

void ufeel_destroy(
    UFeelProcessor* processor
)
{
    guardedVoid("ufeel_destroy", [processor] -> void { delete processor; });
}

auto ufeel_update(
    UFeelProcessor* processor
) -> int32_t
{
    if (!isValidProcessor(processor))
    {
        return 0;
    }

    return guarded("ufeel_update", int32_t{0}, [processor] -> int32_t {
        return static_cast<int32_t>(processor->processor.update());
    });
}

auto ufeel_get_frame(
    UFeelProcessor* processor
) -> UFeelFrame*
{
    if (!isValidProcessor(processor))
    {
        return nullptr;
    }

    return guarded(
        "ufeel_get_frame", static_cast<UFeelFrame*>(nullptr), [processor] -> UFeelFrame* {
            const cv::Mat frame = processor->processor.getFrame();
            if (frame.empty())
            {
                return static_cast<UFeelFrame*>(nullptr);
            }

            if (frame.cols <= 0 || frame.rows <= 0 || frame.channels() <= 0)
            {
                return static_cast<UFeelFrame*>(nullptr);
            }

            const size_t stride = frame.step[0];
            const auto height = static_cast<size_t>(frame.rows);

            if (stride == 0 || stride < static_cast<size_t>(frame.cols) * frame.elemSize() ||
                stride > std::numeric_limits<size_t>::max() / height ||
                stride > static_cast<size_t>(std::numeric_limits<int32_t>::max()))
            {
                return static_cast<UFeelFrame*>(nullptr);
            }

            const size_t byteCount = stride * height;

            auto output = std::unique_ptr<UFeelFrame>(new (std::nothrow) UFeelFrame{});
            if (!output)
            {
                return static_cast<UFeelFrame*>(nullptr);
            }

            output->data = static_cast<uint8_t*>(std::malloc(byteCount));
            if (output->data == nullptr)
            {
                return static_cast<UFeelFrame*>(nullptr);
            }

            std::memcpy(output->data, frame.data, byteCount);

            output->width = frame.cols;
            output->height = frame.rows;
            output->channels = frame.channels();
            output->stride = static_cast<int32_t>(stride);

            return output.release();
        }
    );
}

void ufeel_free_frame(
    UFeelFrame* frame
)
{
    guardedVoid("ufeel_free_frame", [frame] -> void {
        if (frame == nullptr)
        {
            return;
        }

        std::free(frame->data);
        frame->data = nullptr;
        delete frame;
    });
}

auto ufeel_get_emotions(
    UFeelProcessor* processor, int32_t* count
) -> UFeelPair*
{
    if (count == nullptr)
    {
        return nullptr;
    }

    *count = 0;

    if (!isValidProcessor(processor))
    {
        return nullptr;
    }

    return guarded(
        "ufeel_get_emotions", static_cast<UFeelPair*>(nullptr), [processor, count] -> UFeelPair* {
            const auto emotions = processor->processor.getEmotions();

            if (emotions.empty() ||
                emotions.size() > static_cast<size_t>(std::numeric_limits<int32_t>::max()))
            {
                return static_cast<UFeelPair*>(nullptr);
            }

            auto output =
                std::unique_ptr<UFeelPair[]>(new (std::nothrow) UFeelPair[emotions.size()]{});

            if (!output)
            {
                return static_cast<UFeelPair*>(nullptr);
            }

            int32_t index = 0;
            for (const auto& [name, value] : emotions)
            {
                output[index].name = duplicateUtf8(name);
                if (output[index].name == nullptr)
                {
                    for (int32_t allocated = 0; allocated < index; ++allocated)
                    {
                        std::free(output[allocated].name);
                    }

                    return static_cast<UFeelPair*>(nullptr);
                }

                output[index].value = static_cast<float>(value);
                ++index;
            }

            *count = index;
            return output.release();
        }
    );
}

void ufeel_free_emotions(
    UFeelPair* emotions, int32_t count
)
{
    guardedVoid("ufeel_free_emotions", [emotions, count] -> void {
        if (emotions == nullptr)
        {
            return;
        }

        for (int32_t index = 0; index < count; ++index)
        {
            std::free(emotions[index].name);
        }

        delete[] emotions;
    });
}

auto ufeel_begin_eye_tracking_calibration(
    UFeelProcessor* processor
) -> int32_t
{
    if (!isValidProcessor(processor))
    {
        return UFEEL_STATUS_INVALID_ARGUMENT;
    }

    return guarded(
        "ufeel_begin_eye_tracking_calibration",
        int32_t{UFEEL_STATUS_INTERNAL_ERROR},
        [processor] -> UFeelStatus {
            processor->processor.beginEyeTrackingCalibration();
            return UFEEL_STATUS_OK;
        }
    );
}

auto ufeel_set_eye_tracking_calibration_target(
    UFeelProcessor* processor, float screenX, float screenY
) -> int32_t
{
    if (!isValidProcessor(processor))
    {
        return UFEEL_STATUS_INVALID_ARGUMENT;
    }

    if (!std::isfinite(screenX) || !std::isfinite(screenY))
    {
        return UFEEL_STATUS_INVALID_ARGUMENT;
    }

    return guarded(
        "ufeel_set_eye_tracking_calibration_target",
        int32_t{UFEEL_STATUS_INTERNAL_ERROR},
        [processor, screenX, screenY] -> UFeelStatus {
            return processor->processor.setEyeTrackingCalibrationTarget(screenX, screenY)
                       ? UFEEL_STATUS_OK
                       : UFEEL_STATUS_INVALID_STATE;
        }
    );
}

auto ufeel_finish_eye_tracking_calibration(
    UFeelProcessor* processor
) -> int32_t
{
    if (!isValidProcessor(processor))
    {
        return UFEEL_STATUS_INVALID_ARGUMENT;
    }

    return guarded(
        "ufeel_finish_eye_tracking_calibration",
        int32_t{UFEEL_STATUS_INTERNAL_ERROR},
        [processor] -> UFeelStatus {
            return processor->processor.finishEyeTrackingCalibration()
                       ? UFEEL_STATUS_OK
                       : UFEEL_STATUS_EYE_TRACKING_CALIBRATION_FAILED;
        }
    );
}

auto ufeel_cancel_eye_tracking_calibration(
    UFeelProcessor* processor
) -> int32_t
{
    if (!isValidProcessor(processor))
    {
        return UFEEL_STATUS_INVALID_ARGUMENT;
    }

    return guarded(
        "ufeel_cancel_eye_tracking_calibration",
        int32_t{UFEEL_STATUS_INTERNAL_ERROR},
        [processor] -> UFeelStatus {
            processor->processor.cancelEyeTrackingCalibration();
            return UFEEL_STATUS_OK;
        }
    );
}

auto ufeel_get_gaze_result(
    UFeelProcessor* processor, UFeelGazeResult* result
) -> int32_t
{
    if (result == nullptr || !isValidProcessor(processor))
    {
        return UFEEL_STATUS_INVALID_ARGUMENT;
    }

    *result = {};

    return guarded(
        "ufeel_get_gaze_result",
        int32_t{UFEEL_STATUS_INTERNAL_ERROR},
        [processor, result] -> UFeelStatus {
            const auto gaze = processor->processor.getGaze();

            result->status = static_cast<int32_t>(gaze.status);
            result->x = static_cast<float>(gaze.x);
            result->y = static_cast<float>(gaze.y);
            result->recordedX = static_cast<float>(gaze.recordedX);
            result->recordedY = static_cast<float>(gaze.recordedY);
            result->confidence = static_cast<float>(gaze.confidence);
            result->directions = static_cast<int32_t>(gaze.directions);

            return gaze.status == GazeStatus::Valid ? UFEEL_STATUS_OK : UFEEL_STATUS_NO_GAZE;
        }
    );
}

auto ufeel_get_eye_tracking_calibration_progress(
    UFeelProcessor* processor, UFeelEyeTrackingCalibrationProgress* progress
) -> int32_t
{
    if (progress == nullptr || !isValidProcessor(processor))
    {
        return UFEEL_STATUS_INVALID_ARGUMENT;
    }

    *progress = {};

    return guarded(
        "ufeel_get_eye_tracking_calibration_progress",
        int32_t{UFEEL_STATUS_INTERNAL_ERROR},
        [processor, progress] -> UFeelStatus {
            const auto value = processor->processor.getEyeTrackingCalibrationProgress();

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
    if (diagnostics == nullptr || !isValidProcessor(processor))
    {
        return UFEEL_STATUS_INVALID_ARGUMENT;
    }

    *diagnostics = {};

    return guarded(
        "ufeel_get_eye_tracking_calibration_diagnostics",
        int32_t{UFEEL_STATUS_INTERNAL_ERROR},
        [processor, diagnostics] -> UFeelStatus {
            const auto value = processor->processor.getEyeTrackingCalibrationDiagnostics();

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
    if (!isValidProcessor(processor) || utf8Path == nullptr || utf8Path[0] == '\0')
    {
        return UFEEL_STATUS_INVALID_ARGUMENT;
    }

    return guarded(
        "ufeel_save_eye_tracking_calibration_profile",
        int32_t{UFEEL_STATUS_INTERNAL_ERROR},
        [processor, utf8Path] -> UFeelStatus {
            return processor->processor.saveEyeTrackingCalibrationProfile(utf8Path)
                       ? UFEEL_STATUS_OK
                       : UFEEL_STATUS_EYE_TRACKING_PROFILE_FAILED;
        }
    );
}

auto ufeel_load_eye_tracking_calibration_profile(
    UFeelProcessor* processor, const char* utf8Path
) -> int32_t
{
    if (!isValidProcessor(processor) || utf8Path == nullptr || utf8Path[0] == '\0')
    {
        return UFEEL_STATUS_INVALID_ARGUMENT;
    }

    return guarded(
        "ufeel_load_eye_tracking_calibration_profile",
        int32_t{UFEEL_STATUS_INTERNAL_ERROR},
        [processor, utf8Path] -> UFeelStatus {
            return processor->processor.loadEyeTrackingCalibrationProfile(utf8Path)
                       ? UFEEL_STATUS_OK
                       : UFEEL_STATUS_EYE_TRACKING_PROFILE_FAILED;
        }
    );
}

auto ufeel_set_speech_to_text_enabled(
    UFeelProcessor* processor, int32_t enabled
) -> int32_t
{
    if (!isValidProcessor(processor))
    {
        return UFEEL_STATUS_INVALID_ARGUMENT;
    }

    return guarded(
        "ufeel_set_speech_to_text_enabled",
        int32_t{UFEEL_STATUS_INTERNAL_ERROR},
        [processor, enabled] -> UFeelStatus {
            processor->processor.toggleSpeechToText(enabled != 0);
            return UFEEL_STATUS_OK;
        }
    );
}

auto ufeel_get_speech_to_text(
    UFeelProcessor* processor
) -> char*
{
    if (!isValidProcessor(processor))
    {
        return nullptr;
    }

    return guarded("ufeel_get_speech_to_text", static_cast<char*>(nullptr), [processor] -> char* {
        return duplicateUtf8(processor->processor.getSpeech());
    });
}

void ufeel_free_string(
    char* value
)
{
    guardedVoid("ufeel_free_string", [value] -> void { std::free(value); });
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
