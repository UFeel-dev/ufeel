using System;
using System.Collections.Generic;
using System.Linq;
using UFeel;

namespace UFeelTest;

internal static class DisplayFormatter
{
    public static string FormatEmotions(EmotionResultData result)
    {
        if (!result.IsValid)
        {
            return result.Status switch
            {
                EmotionStatus.FaceUnavailable => "Face not detected",
                EmotionStatus.FrameUnavailable => "Camera frame unavailable",
                _ => "Emotion data unavailable"
            };
        }

        if (result.Emotions.Count == 0)
            return "No data";

        return string.Join(
            Environment.NewLine,
            result.Emotions.OrderByDescending(pair => pair.Value)
                .Select(pair => $"{pair.Key}: {pair.Value:F3}"));
    }

    public static string FormatSpeech(SpeechToTextResultData result)
    {
        return result.Status switch
        {
            SpeechToTextStatus.Disabled => "Speech recognition is disabled",
            SpeechToTextStatus.NoSpeech => "No speech detected",
            SpeechToTextStatus.Valid => result.Text,
            _ => "Speech result unavailable"
        };
    }

    public static string FormatDirections(EyeTrackingResultData eyeTracking)
    {
        if (!eyeTracking.IsValid)
        {
            return eyeTracking.Status switch
            {
                EyeTrackingStatus.NotCalibrated => "Not calibrated",
                EyeTrackingStatus.FaceUnavailable => "Face not detected",
                EyeTrackingStatus.Blinking => "Blink detected",
                EyeTrackingStatus.LowQuality => "Eye landmarks are low quality",
                EyeTrackingStatus.Calibrating => "Calibration in progress",
                EyeTrackingStatus.Disabled => "Eye tracking is disabled",
                EyeTrackingStatus.FrameUnavailable => "Camera frame unavailable",
                _ => "EyeTracking unavailable"
            };
        }

        var directions = new List<string>();
        if (eyeTracking.Directions.HasFlag(EyeTrackingDirection.Left))
            directions.Add("left");
        if (eyeTracking.Directions.HasFlag(EyeTrackingDirection.Right))
            directions.Add("right");
        if (eyeTracking.Directions.HasFlag(EyeTrackingDirection.Up))
            directions.Add("up");
        if (eyeTracking.Directions.HasFlag(EyeTrackingDirection.Down))
            directions.Add("down");
        if (eyeTracking.Directions.HasFlag(EyeTrackingDirection.Center))
            directions.Add("center");

        return directions.Count == 0
            ? "No direction"
            : string.Join(", ", directions);
    }

    public static string FormatEyeTracking(EyeTrackingResultData eyeTracking)
    {
        return eyeTracking.IsValid
            ? $"Recorded: ({eyeTracking.RecordedX:F3}, {eyeTracking.RecordedY:F3})" +
            Environment.NewLine +
            $"Estimated: ({eyeTracking.EstimatedX:F3}, {eyeTracking.EstimatedY:F3})" +
            Environment.NewLine +
            $"Confidence: {eyeTracking.Confidence:P0}"
            : "Screen position unavailable";
    }

    public static string FormatHeartRate(HeartRateResultData heartRate)
    {
        if (!heartRate.IsValid)
        {
            return heartRate.Status switch
            {
                HeartRateStatus.JsonParseError => "JSON parse error",
                HeartRateStatus.JsonParseOutOfRange => "JSON parse - out of range",
                HeartRateStatus.RequestFailed => "Request failed",
                HeartRateStatus.NoData => "No data",
                _ => "Heartrate unavailable"
            };
        }

        return heartRate.Bpm.ToString();
    }

    public static string FormatCalibrationFailure(
        EyeTrackingCalibrationDiagnostics diagnostics)
    {
        string reason = diagnostics.FailureReason switch
        {
            CalibrationFailureReason.NotCalibrating =>
                "Calibration was not active. Start a new calibration.",

            CalibrationFailureReason.TooFewValidTargets =>
                "Not enough valid calibration targets were collected.",

            CalibrationFailureReason.InsufficientSamples =>
                "There were not enough usable samples. Keep your head steady and look directly at each target.",

            CalibrationFailureReason.InsufficientHorizontalCoverage =>
                "The calibration targets did not cover enough of the screen horizontally.",

            CalibrationFailureReason.InsufficientVerticalCoverage =>
                "The calibration targets did not cover enough of the screen vertically.",

            CalibrationFailureReason.InsufficientSpatialGridCoverage =>
                "The targets were not distributed sufficiently across the screen.",

            CalibrationFailureReason.RegressionFailed =>
                "The calibration model could not be fitted. Repeat calibration.",

            CalibrationFailureReason.CrossValidationMeanErrorExceeded =>
                "Average calibration error was too high. Repeat calibration while looking steadily at each target.",

            CalibrationFailureReason.CrossValidationMaxErrorExceeded =>
                "At least one target had excessive prediction error. Repeat calibration and focus on each target.",

            CalibrationFailureReason.InvalidCrossValidationResult =>
                "Calibration validation produced an invalid result. Repeat calibration.",

            _ =>
                "Calibration failed without a specific failure reason."
        };

        var details = new List<string>
        {
            $"Failure: {reason}",
            $"Valid targets: {diagnostics.ValidTargetCount}/{diagnostics.InputTargetCount}",
            $"Targets with enough raw samples: {diagnostics.TargetsWithEnoughRawSamples}",
            $"Targets with enough retained samples: {diagnostics.TargetsWithEnoughRetainedSamples}"
        };

        if (diagnostics.InvalidTargetCount > 0)
            details.Add($"Invalid targets: {diagnostics.InvalidTargetCount}");

        if (diagnostics.TargetsWithInsufficientRawSamples > 0)
        {
            details.Add(
                $"Targets with insufficient raw samples: {diagnostics.TargetsWithInsufficientRawSamples}");
        }

        if (diagnostics.TargetsWithInsufficientRetainedSamples > 0)
        {
            details.Add(
                $"Targets with insufficient retained samples: {diagnostics.TargetsWithInsufficientRetainedSamples}");
        }

        if (diagnostics.MinimumRetainedSamples > 0 ||
            diagnostics.MaximumRetainedSamples > 0)
        {
            details.Add(
                $"Retained samples per target: {diagnostics.MinimumRetainedSamples}–{diagnostics.MaximumRetainedSamples}");
        }

        details.Add(
            $"Horizontal span: {diagnostics.HorizontalSpan:F3}; vertical span: {diagnostics.VerticalSpan:F3}");

        if (diagnostics.CrossValidationTargetsEvaluated > 0)
        {
            details.Add(
                $"Cross-validation targets evaluated: {diagnostics.CrossValidationTargetsEvaluated}");

            details.Add(
                $"Mean cross-validation error: {diagnostics.MeanCrossValidationError:F3}");

            details.Add(
                $"Maximum cross-validation error: {diagnostics.MaximumCrossValidationError:F3}");
        }

        if (diagnostics.RegressionFailures > 0)
            details.Add($"Regression failures: {diagnostics.RegressionFailures}");

        if (diagnostics.MeanErrorExceeded)
            details.Add("The mean-error threshold was exceeded.");

        if (diagnostics.MaximumErrorExceeded)
            details.Add("The maximum-error threshold was exceeded.");

        return string.Join(Environment.NewLine, details);
    }
}
