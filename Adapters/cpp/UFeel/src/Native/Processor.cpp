#include "ufeel/Native/Processor.hpp"

#include "ufeel.h"
#include "ufeel/Configuration/PackageLayout.hpp"
#include "ufeel/Model/Calibration.hpp"
#include "ufeel/Model/Detection.hpp"
#include "ufeel/Model/EmotionResult.hpp"
#include "ufeel/Model/EyeTrackingResult.hpp"
#include "ufeel/Model/Frame.hpp"
#include "ufeel/Model/HeartRateResult.hpp"
#include "ufeel/Model/Microphone.hpp"
#include "ufeel/Model/SpeechToTextResult.hpp"
#include "ufeel/Model/Status.hpp"
#include "ufeel/Native/Error.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace ufeel
{

namespace
{

template <auto Free> struct NativeDeleter
{
    template <typename Result>
    void operator()(
        Result* result
    ) const noexcept
    {
        Free(result);
    }
};

template <auto Create, auto Free> auto createNativeResult()
{
    using Result = std::remove_pointer_t<decltype(Create())>;

    std::unique_ptr<Result, NativeDeleter<Free>> result(Create());
    if (!result)
    {
        throw std::bad_alloc();
    }
    return result;
}

auto malformed(
    std::string_view operation, std::string_view reason
) -> Error
{
    return {Status::InternalError, operation, reason};
}

constexpr auto isKnown(
    Status status
) -> bool
{
    switch (status)
    {
    case Status::Ok:
    case Status::NoEmotions:
    case Status::NoEyeTracking:
    case Status::NoSpeech:
    case Status::NoHeartRate:
    case Status::InvalidArgument:
    case Status::InvalidState:
    case Status::EyeTrackingCalibrationFailed:
    case Status::EyeTrackingProfileFailed:
    case Status::InternalError:
    case Status::InvalidProcessor:
    case Status::InvalidFrame:
    case Status::UnsupportedFeature:
        return true;
    }
    return false;
}

constexpr auto isKnown(
    EmotionStatus status
) -> bool
{
    switch (status)
    {
    case EmotionStatus::FaceUnavailable:
    case EmotionStatus::Valid:
    case EmotionStatus::FrameUnavailable:
    case EmotionStatus::InvalidProcessor:
    case EmotionStatus::InternalError:
    case EmotionStatus::UnsupportedFeature:
        return true;
    }
    return false;
}

constexpr auto isKnown(
    EyeTrackingStatus status
) -> bool
{
    switch (status)
    {
    case EyeTrackingStatus::NotCalibrated:
    case EyeTrackingStatus::FaceUnavailable:
    case EyeTrackingStatus::LowQuality:
    case EyeTrackingStatus::Calibrating:
    case EyeTrackingStatus::Valid:
    case EyeTrackingStatus::Disabled:
    case EyeTrackingStatus::FrameUnavailable:
    case EyeTrackingStatus::InvalidProcessor:
    case EyeTrackingStatus::InternalError:
        return true;
    }
    return false;
}

constexpr auto isKnown(
    SpeechToTextStatus status
) -> bool
{
    switch (status)
    {
    case SpeechToTextStatus::Disabled:
    case SpeechToTextStatus::NoSpeech:
    case SpeechToTextStatus::Valid:
        return true;
    }
    return false;
}

constexpr auto isKnown(
    HeartRateStatus status
) -> bool
{
    switch (status)
    {
    case HeartRateStatus::JsonParseError:
    case HeartRateStatus::JsonParseOutOfRange:
    case HeartRateStatus::RequestFailed:
    case HeartRateStatus::Valid:
    case HeartRateStatus::NoData:
        return true;
    }
    return false;
}

constexpr auto isKnown(
    CalibrationFailureReason reason
) -> bool
{
    switch (reason)
    {
    case CalibrationFailureReason::None:
    case CalibrationFailureReason::NotCalibrating:
    case CalibrationFailureReason::TooFewValidTargets:
    case CalibrationFailureReason::InsufficientSamples:
    case CalibrationFailureReason::InsufficientHorizontalCoverage:
    case CalibrationFailureReason::InsufficientVerticalCoverage:
    case CalibrationFailureReason::InsufficientSpatialGridCoverage:
    case CalibrationFailureReason::RegressionFailed:
    case CalibrationFailureReason::CrossValidationMeanErrorExceeded:
    case CalibrationFailureReason::CrossValidationMaxErrorExceeded:
    case CalibrationFailureReason::InvalidCrossValidationResult:
        return true;
    }
    return false;
}

template <typename Enum>
auto toKnown(
    std::int32_t value, std::string_view operation, std::string_view field
) -> Enum
{
    if (std::in_range<std::underlying_type_t<Enum>>(value))
    {
        const auto candidate = static_cast<Enum>(value);
        if (isKnown(candidate))
        {
            return candidate;
        }
    }

    throw malformed(
        operation, std::string("unknown ").append(field).append(" ").append(std::to_string(value))
    );
}

auto checkStatus(
    std::int32_t native, std::string_view operation
) -> Status
{
    return toKnown<Status>(native, operation, "status");
}

void throwUnless(
    Status status, std::string_view operation, Status accepted = Status::Ok
)
{
    if (status != Status::Ok && status != accepted)
    {
        throw Error(status, operation);
    }
}

void require(
    std::int32_t native, std::string_view operation, Status accepted = Status::Ok
)
{
    throwUnless(checkStatus(native, operation), operation, accepted);
}

auto toUtf8(
    const std::filesystem::path& path
) -> std::string
{
    const auto utf8 = path.u8string();
    return {utf8.begin(), utf8.end()};
}

void requirePath(
    const std::filesystem::path& path, std::string_view description
)
{
    if (path.empty())
    {
        throw std::invalid_argument(
            std::string("A non-empty ").append(description).append(" is required.")
        );
    }
}

auto createNative(
    const std::filesystem::path& modelDirectory, const std::filesystem::path& speechModelPath
) -> UFeelProcessor*
{
    requirePath(modelDirectory, "model directory");
    requirePath(speechModelPath, "speech model path");

    const std::string models = toUtf8(modelDirectory);
    auto* handle = ufeel_create(models.c_str(), toUtf8(speechModelPath).c_str());
    if (handle == nullptr)
    {
        throw Error(
            Status::InternalError,
            "create the native processor",
            "the models in " + models + " could not be loaded"
        );
    }
    return handle;
}

template <typename Item>
auto items(
    Item* data, std::int32_t count, std::string_view operation
) -> std::span<const Item>
{
    if (count < 0 || (count > 0 && data == nullptr))
    {
        throw malformed(operation, "invalid item buffer");
    }
    return {data, static_cast<std::size_t>(count)};
}

} // namespace

void Processor::Destroy::operator()(
    UFeelProcessor* handle
) const noexcept
{
    ufeel_destroy(handle);
}

Processor::Processor(
    const PackageLayout& layout
)
    : Processor(layout.modelDirectory, layout.speechModelPath)
{
}

Processor::Processor(
    const std::filesystem::path& modelDirectory, const std::filesystem::path& speechModelPath
)
    : Processor(createNative(modelDirectory, speechModelPath))
{
}

Processor::Processor(
    UFeelProcessor* handle
)
    : handle_(handle)
{
}

auto Processor::adopt(
    UFeelProcessor* handle
) -> Processor
{
    if (handle == nullptr)
    {
        throw std::invalid_argument("Cannot adopt a null UFeel processor.");
    }
    return Processor(handle);
}

auto Processor::handle() const -> UFeelProcessor*
{
    if (!handle_)
    {
        throw std::logic_error("The UFeel processor was moved from.");
    }
    return handle_.get();
}

auto Processor::update(
    DetectionType features
) -> Status
{
    return checkStatus(
        ufeel_update(handle(), static_cast<std::int32_t>(features)), "update the processor"
    );
}

auto Processor::updateFromFrame(
    DetectionType features, const Frame& frame
) -> Status
{
    const UFeelFrame native{
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast): input frames are only read
        .data = const_cast<std::uint8_t*>(frame.data.data()),
        .width = frame.width,
        .height = frame.height,
        .channels = frame.channels,
        .stride = frame.stride,
        .dataSize = frame.data.size(),
    };

    return checkStatus(
        ufeel_update_from_frame(handle(), static_cast<std::int32_t>(features), &native),
        "update the processor from a frame"
    );
}

auto Processor::updateFrame() -> Status
{
    return checkStatus(ufeel_update_frame(handle()), "capture a camera frame");
}

auto Processor::frameSize() -> std::optional<FrameSize>
{
    UFeelFrame info{};
    if (checkStatus(ufeel_get_frame_info(handle(), &info), "read the frame size") != Status::Ok ||
        info.width <= 0 || info.height <= 0)
    {
        return std::nullopt;
    }
    return FrameSize{.width = info.width, .height = info.height};
}

auto Processor::copyFrame(
    Frame& destination
) -> bool
{
    UFeelFrame info{};
    if (checkStatus(ufeel_get_frame_info(handle(), &info), "read the frame size") != Status::Ok ||
        info.width <= 0 || info.height <= 0 || info.channels <= 0 || info.stride <= 0)
    {
        return false;
    }

    const auto expectedSize =
        static_cast<std::uint64_t>(info.stride) * static_cast<std::uint64_t>(info.height);
    constexpr auto maximumSize =
        static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max());
    if (info.dataSize < expectedSize || info.dataSize > maximumSize)
    {
        return false;
    }

    destination.data.resize(static_cast<std::size_t>(info.dataSize));
    destination.width = info.width;
    destination.height = info.height;
    destination.channels = info.channels;
    destination.stride = info.stride;

    return checkStatus(
               ufeel_copy_frame(
                   handle(), destination.data.data(), static_cast<std::int32_t>(info.dataSize)
               ),
               "copy the frame"
           ) == Status::Ok;
}

auto Processor::copyFrameBgra(
    std::span<std::uint8_t> destination, int rowBytes, FrameSize size
) -> bool
{
    constexpr std::int64_t bytesPerPixel = 4;
    if (size.width <= 0 || size.height <= 0 ||
        static_cast<std::int64_t>(rowBytes) <
            static_cast<std::int64_t>(size.width) * bytesPerPixel ||
        destination.size() <
            static_cast<std::size_t>(rowBytes) * static_cast<std::size_t>(size.height))
    {
        throw std::invalid_argument(
            "The BGRA destination needs rowBytes >= width * 4 and rowBytes * height bytes."
        );
    }

    return checkStatus(
               ufeel_copy_frame_bgra(
                   handle(), destination.data(), rowBytes, size.width, size.height
               ),
               "copy the frame as BGRA"
           ) == Status::Ok;
}

auto Processor::emotions() -> EmotionResult
{
    constexpr std::string_view operation = "retrieve emotion estimates";

    const auto result =
        createNativeResult<ufeel_create_emotions_result, ufeel_free_emotions_result>();
    const Status status = checkStatus(ufeel_get_emotions_result(handle(), result.get()), operation);
    if (status != Status::NoEmotions && status != Status::InvalidFrame)
    {
        throwUnless(status, operation);
    }

    EmotionResult emotions{
        .status = toKnown<EmotionStatus>(result->status, operation, "emotion status"),
        .scores = {},
    };
    for (const UFeelPair& pair : items(result->emotions, result->count, operation))
    {
        if (pair.name == nullptr)
        {
            throw malformed(operation, "null emotion label");
        }
        emotions.scores.insert_or_assign(pair.name, pair.value);
    }
    return emotions;
}

auto Processor::eyeTracking() -> EyeTrackingResult
{
    constexpr std::string_view operation = "retrieve the gaze estimate";

    UFeelEyeTrackingResult native{};
    require(ufeel_get_eye_tracking_result(handle(), &native), operation, Status::NoEyeTracking);

    return {
        .status = toKnown<EyeTrackingStatus>(native.status, operation, "eye-tracking status"),
        .estimatedX = native.x,
        .estimatedY = native.y,
        .recordedX = native.recordedX,
        .recordedY = native.recordedY,
        .confidence = native.confidence,
        .directions = static_cast<EyeTrackingDirection>(native.directions),
        .gazeX = native.gazeX,
        .gazeY = native.gazeY,
        .leftOpenness = native.leftOpenness,
        .rightOpenness = native.rightOpenness,
        .eyeState = static_cast<EyeState>(native.eyeState),
        .blinkCount = native.blinkCount,
        .eyesClosedSeconds = native.eyesClosedSeconds,
    };
}

auto Processor::speechToText() -> SpeechToTextResult
{
    constexpr std::string_view operation = "retrieve the speech-to-text result";

    const auto result =
        createNativeResult<ufeel_create_speech_to_text_result, ufeel_free_speech_to_text_result>();
    require(ufeel_get_speech_to_text_result(handle(), result.get()), operation, Status::NoSpeech);
    if (result->speech == nullptr)
    {
        throw malformed(operation, "no text buffer");
    }

    return {
        .status = toKnown<SpeechToTextStatus>(result->status, operation, "speech status"),
        .text = result->speech,
    };
}

auto Processor::heartRate() -> HeartRateResult
{
    constexpr std::string_view operation = "retrieve the heart rate";

    UFeelHeartRateResult native{};
    require(ufeel_get_heart_rate_result(handle(), &native), operation, Status::NoHeartRate);

    return {
        .status = toKnown<HeartRateStatus>(native.status, operation, "heart-rate status"),
        .bpm = native.bpm,
    };
}

void Processor::setSpeechToTextEnabled(
    bool enabled
)
{
    require(
        ufeel_set_speech_to_text_enabled(handle(), enabled ? 1 : 0),
        "change the speech-to-text state"
    );
}

void Processor::setEyeTrackingHeadWeight(
    float weight
)
{
    require(
        ufeel_set_eye_tracking_head_weight(handle(), weight), "set the eye-tracking head weight"
    );
}

auto Processor::microphones() -> std::vector<Microphone>
{
    constexpr std::string_view operation = "list the microphones";

    const auto result =
        createNativeResult<ufeel_create_microphones_result, ufeel_free_microphones_result>();
    const Status status =
        checkStatus(ufeel_get_microphones_result(handle(), result.get()), operation);
    if (status == Status::UnsupportedFeature)
    {
        return {};
    }
    throwUnless(status, operation);

    std::vector<Microphone> microphones;
    for (const UFeelMicrophone& microphone : items(result->microphones, result->count, operation))
    {
        if (microphone.id == nullptr || microphone.name == nullptr)
        {
            throw malformed(operation, "null microphone id or name");
        }
        microphones.push_back({
            .id = microphone.id,
            .name = microphone.name,
            .isDefault = microphone.isDefault != 0,
        });
    }
    return microphones;
}

auto Processor::selectMicrophone(
    const std::string& id
) -> bool
{
    constexpr std::string_view operation = "select the microphone";

    const Status status =
        checkStatus(ufeel_set_microphone(handle(), id.empty() ? nullptr : id.c_str()), operation);
    if (status == Status::InvalidArgument)
    {
        return false;
    }
    throwUnless(status, operation);
    return true;
}

auto Processor::haveMicrophonesChanged() -> bool
{
    constexpr std::string_view operation = "check the microphones";

    std::int32_t changed = 0;
    const Status status = checkStatus(ufeel_get_microphones_changed(handle(), &changed), operation);
    if (status == Status::UnsupportedFeature)
    {
        return false;
    }
    throwUnless(status, operation);
    return changed != 0;
}

void Processor::beginEyeTrackingCalibration()
{
    require(ufeel_begin_eye_tracking_calibration(handle()), "begin eye-tracking calibration");
}

void Processor::setEyeTrackingCalibrationTarget(
    float screenX, float screenY
)
{
    require(
        ufeel_set_eye_tracking_calibration_target(handle(), screenX, screenY),
        "set the eye-tracking calibration target"
    );
}

auto Processor::finishEyeTrackingCalibration() -> bool
{
    constexpr std::string_view operation = "finish eye-tracking calibration";

    const Status status = checkStatus(ufeel_finish_eye_tracking_calibration(handle()), operation);
    if (status == Status::EyeTrackingCalibrationFailed)
    {
        return false;
    }
    throwUnless(status, operation);
    return true;
}

void Processor::cancelEyeTrackingCalibration()
{
    require(ufeel_cancel_eye_tracking_calibration(handle()), "cancel eye-tracking calibration");
}

auto Processor::eyeTrackingCalibrationProgress() -> CalibrationProgress
{
    UFeelEyeTrackingCalibrationProgress native{};
    require(
        ufeel_get_eye_tracking_calibration_progress(handle(), &native),
        "retrieve eye-tracking calibration progress"
    );

    return {
        .targetsWithEnoughSamples = native.targetsWithEnoughSamples,
        .targetCount = native.targetCount,
        .samplesForCurrentTarget = native.samplesForCurrentTarget,
        .requiredSamplesPerTarget = native.requiredSamplesPerTarget,
    };
}

auto Processor::eyeTrackingCalibrationDiagnostics() -> CalibrationDiagnostics
{
    constexpr std::string_view operation = "retrieve eye-tracking calibration diagnostics";

    UFeelEyeTrackingCalibrationDiagnostics native{};
    require(ufeel_get_eye_tracking_calibration_diagnostics(handle(), &native), operation);

    return {
        .failureReason =
            toKnown<CalibrationFailureReason>(native.failureReason, operation, "failure reason"),
        .inputTargetCount = native.inputTargetCount,
        .targetsWithEnoughRawSamples = native.targetsWithEnoughRawSamples,
        .targetsWithEnoughRetainedSamples = native.targetsWithEnoughRetainedSamples,
        .validTargetCount = native.validTargetCount,
        .minimumRetainedSamples = native.minimumRetainedSamples,
        .maximumRetainedSamples = native.maximumRetainedSamples,
        .horizontalSpan = native.horizontalSpan,
        .verticalSpan = native.verticalSpan,
        .meanCrossValidationError = native.meanCrossValidationError,
        .maximumCrossValidationError = native.maximumCrossValidationError,
        .crossValidationTargetsEvaluated = native.crossValidationTargetsEvaluated,
        .meanErrorExceeded = native.meanErrorExceeded != 0,
        .maximumErrorExceeded = native.maximumErrorExceeded != 0,
        .regressionFailures = native.regressionFailures,
        .invalidTargetCount = native.invalidTargetCount,
        .targetsWithInsufficientRawSamples = native.targetsWithInsufficientRawSamples,
        .targetsWithInsufficientRetainedSamples = native.targetsWithInsufficientRetainedSamples,
    };
}

auto Processor::saveEyeTrackingCalibrationProfile(
    const std::filesystem::path& path
) -> bool
{
    constexpr std::string_view operation = "save the eye-tracking calibration profile";

    requirePath(path, "profile path");
    const Status status = checkStatus(
        ufeel_save_eye_tracking_calibration_profile(handle(), toUtf8(path).c_str()), operation
    );
    if (status == Status::EyeTrackingProfileFailed)
    {
        return false;
    }
    throwUnless(status, operation);
    return true;
}

auto Processor::loadEyeTrackingCalibrationProfile(
    const std::filesystem::path& path
) -> bool
{
    constexpr std::string_view operation = "load the eye-tracking calibration profile";

    requirePath(path, "profile path");
    const Status status = checkStatus(
        ufeel_load_eye_tracking_calibration_profile(handle(), toUtf8(path).c_str()), operation
    );
    if (status == Status::EyeTrackingProfileFailed)
    {
        return false;
    }
    throwUnless(status, operation);
    return true;
}

} // namespace ufeel
