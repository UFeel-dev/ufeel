#include "ufeel/Text/Text.hpp"

#include "ufeel/Model/Calibration.hpp"
#include "ufeel/Model/EmotionResult.hpp"
#include "ufeel/Model/EyeTrackingResult.hpp"
#include "ufeel/Model/Flags.hpp"
#include "ufeel/Model/HeartRateResult.hpp"
#include "ufeel/Model/Microphone.hpp"
#include "ufeel/Model/SpeechToTextResult.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace ufeel::text
{

namespace
{

auto fixed(
    float value, int precision
) -> std::string
{
    std::array<char, 64> buffer{};
    const auto [end, error] = std::to_chars(
        buffer.data(), buffer.data() + buffer.size(), value, std::chars_format::fixed, precision
    );
    if (error != std::errc{})
    {
        return "?";
    }
    return {buffer.data(), end};
}

auto signedFixed(
    float value
) -> std::string
{
    const std::string magnitude = fixed(std::abs(value), 2);
    const bool negative = value < 0.0F && magnitude != fixed(0.0F, 2);
    return (negative ? "-" : "+") + magnitude;
}

auto join(
    std::span<const std::string> parts, std::string_view separator
) -> std::string
{
    std::string joined;
    for (const auto& part : parts)
    {
        if (!joined.empty())
        {
            joined.append(separator);
        }
        joined.append(part);
    }
    return joined;
}

auto emotionsUnavailable(
    EmotionStatus status
) -> std::string_view
{
    switch (status)
    {
    case EmotionStatus::FaceUnavailable:
        return "Face not detected";
    case EmotionStatus::FrameUnavailable:
        return "Camera frame unavailable";
    case EmotionStatus::UnsupportedFeature:
        return "Emotion detection is not built in";
    case EmotionStatus::Valid:
    case EmotionStatus::InvalidProcessor:
    case EmotionStatus::InternalError:
        break;
    }
    return "Emotion data unavailable";
}

auto eyeTrackingUnavailable(
    EyeTrackingStatus status
) -> std::string_view
{
    switch (status)
    {
    case EyeTrackingStatus::FaceUnavailable:
        return "Face not detected";
    case EyeTrackingStatus::LowQuality:
        return "Eye landmarks are low quality";
    case EyeTrackingStatus::Disabled:
        return "Eye tracking is disabled";
    case EyeTrackingStatus::FrameUnavailable:
        return "Camera frame unavailable";
    case EyeTrackingStatus::NotCalibrated:
    case EyeTrackingStatus::Calibrating:
    case EyeTrackingStatus::Valid:
    case EyeTrackingStatus::InvalidProcessor:
    case EyeTrackingStatus::InternalError:
        break;
    }
    return "Eye tracking unavailable";
}

} // namespace

auto emotions(
    const EmotionResult& result
) -> std::string
{
    if (!result.isValid())
    {
        return std::string(emotionsUnavailable(result.status));
    }

    if (result.scores.empty())
    {
        return "No data";
    }

    std::vector<std::pair<std::string_view, float>> scores(
        result.scores.begin(), result.scores.end()
    );
    std::ranges::stable_sort(scores, std::ranges::greater{}, [](const auto& score) -> float {
        return score.second;
    });

    std::vector<std::string> lines;
    lines.reserve(scores.size());
    for (const auto& [label, score] : scores)
    {
        lines.push_back(std::string(label).append(": ").append(fixed(score, 3)));
    }
    return join(lines, "\n");
}

auto dominantEmotion(
    const EmotionResult& result
) -> std::string
{
    const auto dominant = result.dominant();
    return dominant ? std::string(toString(*dominant)) : "None";
}

auto speechToText(
    const SpeechToTextResult& result
) -> std::string
{
    switch (result.status)
    {
    case SpeechToTextStatus::Disabled:
        return "Speech recognition is disabled";
    case SpeechToTextStatus::NoSpeech:
        return "No speech detected";
    case SpeechToTextStatus::Valid:
        return result.text;
    }
    return "Speech result unavailable";
}

auto defaultMicrophone(
    std::span<const Microphone> microphones
) -> std::string
{
    for (const Microphone& microphone : microphones)
    {
        if (microphone.isDefault)
        {
            return "Default microphone (" + microphone.name + ")";
        }
    }
    return "Default microphone";
}

auto directions(
    const EyeTrackingResult& result
) -> std::string
{
    if (!result.isTracked())
    {
        return std::string(eyeTrackingUnavailable(result.status));
    }

    if (result.areEyesClosed())
    {
        return "Eyes closed";
    }

    constexpr std::array<std::pair<EyeTrackingDirection, std::string_view>, 5> names{
        {
            {EyeTrackingDirection::Left, "left"},
            {EyeTrackingDirection::Right, "right"},
            {EyeTrackingDirection::Up, "up"},
            {EyeTrackingDirection::Down, "down"},
            {EyeTrackingDirection::Center, "center"},
        },
    };

    std::vector<std::string> parts;
    for (const auto& [direction, name] : names)
    {
        if (hasAny(result.directions, direction))
        {
            parts.emplace_back(name);
        }
    }

    return parts.empty() ? "No direction" : join(parts, ", ");
}

auto eyes(
    const EyeTrackingResult& result
) -> std::string
{
    if (!result.isTracked())
    {
        return {};
    }

    return "Gaze: (" + signedFixed(result.gazeX) + ", " + signedFixed(result.gazeY) +
           ")\nOpenness: L " + fixed(result.leftOpenness, 2) + " R " +
           fixed(result.rightOpenness, 2) + "\nBlinks: " + std::to_string(result.blinkCount);
}

auto eyeTracking(
    const EyeTrackingResult& result
) -> std::string
{
    if (!result.isValid())
    {
        return "Screen position unavailable";
    }

    return "Recorded: (" + fixed(result.recordedX, 3) + ", " + fixed(result.recordedY, 3) +
           ")\nEstimated: (" + fixed(result.estimatedX, 3) + ", " + fixed(result.estimatedY, 3) +
           ")\nConfidence: " + fixed(result.confidence * 100.0F, 0) + " %";
}

auto heartRate(
    const HeartRateResult& result
) -> std::string
{
    switch (result.status)
    {
    case HeartRateStatus::Valid:
        return std::to_string(result.bpm);
    case HeartRateStatus::JsonParseError:
        return "JSON parse error";
    case HeartRateStatus::JsonParseOutOfRange:
        return "JSON parse - out of range";
    case HeartRateStatus::RequestFailed:
        return "Request failed";
    case HeartRateStatus::NoData:
        return "No data";
    }
    return "Heart rate unavailable";
}

auto calibrationFailure(
    const CalibrationDiagnostics& diagnostics
) -> std::string
{
    std::vector<std::string> lines{
        "Failure: " + std::string(failureReason(diagnostics.failureReason)),
        "Valid targets: " + std::to_string(diagnostics.validTargetCount) + "/" +
            std::to_string(diagnostics.inputTargetCount),
        "Targets with enough raw samples: " +
            std::to_string(diagnostics.targetsWithEnoughRawSamples),
        "Targets with enough retained samples: " +
            std::to_string(diagnostics.targetsWithEnoughRetainedSamples),
    };

    if (diagnostics.invalidTargetCount > 0)
    {
        lines.push_back("Invalid targets: " + std::to_string(diagnostics.invalidTargetCount));
    }

    if (diagnostics.targetsWithInsufficientRawSamples > 0)
    {
        lines.push_back(
            "Targets with insufficient raw samples: " +
            std::to_string(diagnostics.targetsWithInsufficientRawSamples)
        );
    }

    if (diagnostics.targetsWithInsufficientRetainedSamples > 0)
    {
        lines.push_back(
            "Targets with insufficient retained samples: " +
            std::to_string(diagnostics.targetsWithInsufficientRetainedSamples)
        );
    }

    if (diagnostics.minimumRetainedSamples > 0 || diagnostics.maximumRetainedSamples > 0)
    {
        lines.push_back(
            "Retained samples per target: " + std::to_string(diagnostics.minimumRetainedSamples) +
            "-" + std::to_string(diagnostics.maximumRetainedSamples)
        );
    }

    lines.push_back(
        "Horizontal span: " + fixed(diagnostics.horizontalSpan, 3) +
        "; vertical span: " + fixed(diagnostics.verticalSpan, 3)
    );

    if (diagnostics.crossValidationTargetsEvaluated > 0)
    {
        lines.push_back(
            "Cross-validation targets evaluated: " +
            std::to_string(diagnostics.crossValidationTargetsEvaluated)
        );
        lines.push_back(
            "Mean cross-validation error: " + fixed(diagnostics.meanCrossValidationError, 3)
        );
        lines.push_back(
            "Maximum cross-validation error: " + fixed(diagnostics.maximumCrossValidationError, 3)
        );
    }

    if (diagnostics.regressionFailures > 0)
    {
        lines.push_back("Regression failures: " + std::to_string(diagnostics.regressionFailures));
    }

    if (diagnostics.meanErrorExceeded)
    {
        lines.emplace_back("The mean-error threshold was exceeded.");
    }

    if (diagnostics.maximumErrorExceeded)
    {
        lines.emplace_back("The maximum-error threshold was exceeded.");
    }

    return join(lines, "\n");
}

auto failureReason(
    CalibrationFailureReason reason
) -> std::string_view
{
    switch (reason)
    {
    case CalibrationFailureReason::NotCalibrating:
        return "Calibration was not active. Start a new calibration.";
    case CalibrationFailureReason::TooFewValidTargets:
        return "Not enough valid calibration targets were collected.";
    case CalibrationFailureReason::InsufficientSamples:
        return "There were not enough usable samples. Keep your head steady and look directly at "
               "each target.";
    case CalibrationFailureReason::InsufficientHorizontalCoverage:
        return "The calibration targets did not cover enough of the screen horizontally.";
    case CalibrationFailureReason::InsufficientVerticalCoverage:
        return "The calibration targets did not cover enough of the screen vertically.";
    case CalibrationFailureReason::InsufficientSpatialGridCoverage:
        return "The targets were not distributed sufficiently across the screen.";
    case CalibrationFailureReason::RegressionFailed:
        return "The calibration model could not be fitted. Repeat calibration.";
    case CalibrationFailureReason::CrossValidationMeanErrorExceeded:
        return "Average calibration error was too high. Repeat calibration while looking steadily "
               "at each target.";
    case CalibrationFailureReason::CrossValidationMaxErrorExceeded:
        return "At least one target had excessive prediction error. Repeat calibration and focus "
               "on each target.";
    case CalibrationFailureReason::InvalidCrossValidationResult:
        return "Calibration validation produced an invalid result. Repeat calibration.";
    case CalibrationFailureReason::None:
        break;
    }
    return "Calibration failed without a specific failure reason.";
}

} // namespace ufeel::text
