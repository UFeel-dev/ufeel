#nullable enable

using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;

namespace UFeel
{
    public static class UFeelText
    {
        public static string Emotions(EmotionResult result)
        {
            if (!result.IsValid)
            {
                return result.Status switch
                {
                    EmotionStatus.FaceUnavailable => "Face not detected",
                    EmotionStatus.FrameUnavailable => "Camera frame unavailable",
                    EmotionStatus.UnsupportedFeature => "Emotion detection is not built in",
                    _ => "Emotion data unavailable",
                };
            }

            if (result.Scores.Count == 0)
                return "No data";

            return string.Join(
                Environment.NewLine,
                result.Scores
                    .OrderByDescending(pair => pair.Value)
                    .Select(pair => string.Format(CultureInfo.InvariantCulture, "{0}: {1:F3}", pair.Key, pair.Value)));
        }

        public static string DominantEmotion(EmotionResult result)
        {
            return result.Dominant is EmotionType dominant ? dominant.ToString() : "None";
        }

        public static string SpeechToText(SpeechToTextResult result)
        {
            return result.Status switch
            {
                SpeechToTextStatus.Disabled => "Speech recognition is disabled",
                SpeechToTextStatus.NoSpeech => "No speech detected",
                SpeechToTextStatus.Valid => result.Text,
                _ => "Speech result unavailable",
            };
        }

        public static string DefaultMicrophone(IReadOnlyList<MicrophoneDevice> microphones)
        {
            if (microphones is null)
                throw new ArgumentNullException(nameof(microphones));

            MicrophoneDevice? device = microphones.FirstOrDefault(microphone => microphone.IsDefault);
            return device is null ? "Default microphone" : $"Default microphone ({device.Name})";
        }

        public static string Directions(EyeTrackingResult result)
        {
            if (!result.IsTracked)
            {
                return result.Status switch
                {
                    EyeTrackingStatus.FaceUnavailable => "Face not detected",
                    EyeTrackingStatus.LowQuality => "Eye landmarks are low quality",
                    EyeTrackingStatus.Disabled => "Eye tracking is disabled",
                    EyeTrackingStatus.FrameUnavailable => "Camera frame unavailable",
                    _ => "Eye tracking unavailable",
                };
            }
            if (result.AreEyesClosed)
                return "Eyes closed";

            var directions = new List<string>(2);
            if (result.Directions.HasDirection(EyeTrackingDirection.Left))
                directions.Add("left");
            if (result.Directions.HasDirection(EyeTrackingDirection.Right))
                directions.Add("right");
            if (result.Directions.HasDirection(EyeTrackingDirection.Up))
                directions.Add("up");
            if (result.Directions.HasDirection(EyeTrackingDirection.Down))
                directions.Add("down");
            if (result.Directions.HasDirection(EyeTrackingDirection.Center))
                directions.Add("center");

            return directions.Count == 0 ? "No direction" : string.Join(", ", directions);
        }

        public static string Eyes(EyeTrackingResult result)
        {
            if (!result.IsTracked)
                return string.Empty;

            return string.Format(
                CultureInfo.InvariantCulture,
                "Gaze: ({0:+0.00;-0.00}, {1:+0.00;-0.00}){5}Openness: L {2:F2} R {3:F2}{5}Blinks: {4}",
                result.GazeX,
                result.GazeY,
                result.LeftOpenness,
                result.RightOpenness,
                result.BlinkCount,
                Environment.NewLine);
        }

        public static string EyeTracking(EyeTrackingResult result)
        {
            if (!result.IsValid)
                return "Screen position unavailable";

            return string.Format(
                CultureInfo.InvariantCulture,
                "Recorded: ({0:F3}, {1:F3}){4}Estimated: ({2:F3}, {3:F3}){4}Confidence: {5:P0}",
                result.RecordedX,
                result.RecordedY,
                result.EstimatedX,
                result.EstimatedY,
                Environment.NewLine,
                result.Confidence);
        }

        public static string HeartRate(HeartRateResult result)
        {
            if (result.IsValid)
                return result.Bpm.ToString(CultureInfo.InvariantCulture);

            return result.Status switch
            {
                HeartRateStatus.JsonParseError => "JSON parse error",
                HeartRateStatus.JsonParseOutOfRange => "JSON parse - out of range",
                HeartRateStatus.RequestFailed => "Request failed",
                HeartRateStatus.NoData => "No data",
                _ => "Heart rate unavailable",
            };
        }

        public static string CalibrationFailure(CalibrationDiagnostics diagnostics)
        {
            if (diagnostics is null)
                throw new ArgumentNullException(nameof(diagnostics));

            var lines = new List<string>
            {
                "Failure: " + FailureReason(diagnostics.FailureReason),
                $"Valid targets: {diagnostics.ValidTargetCount}/{diagnostics.InputTargetCount}",
                $"Targets with enough raw samples: {diagnostics.TargetsWithEnoughRawSamples}",
                $"Targets with enough retained samples: {diagnostics.TargetsWithEnoughRetainedSamples}",
            };

            if (diagnostics.InvalidTargetCount > 0)
                lines.Add($"Invalid targets: {diagnostics.InvalidTargetCount}");

            if (diagnostics.TargetsWithInsufficientRawSamples > 0)
                lines.Add($"Targets with insufficient raw samples: {diagnostics.TargetsWithInsufficientRawSamples}");

            if (diagnostics.TargetsWithInsufficientRetainedSamples > 0)
                lines.Add($"Targets with insufficient retained samples: {diagnostics.TargetsWithInsufficientRetainedSamples}");

            if (diagnostics.MinimumRetainedSamples > 0 || diagnostics.MaximumRetainedSamples > 0)
                lines.Add($"Retained samples per target: {diagnostics.MinimumRetainedSamples}–{diagnostics.MaximumRetainedSamples}");

            lines.Add(string.Format(
                CultureInfo.InvariantCulture,
                "Horizontal span: {0:F3}; vertical span: {1:F3}",
                diagnostics.HorizontalSpan,
                diagnostics.VerticalSpan));

            if (diagnostics.CrossValidationTargetsEvaluated > 0)
            {
                lines.Add($"Cross-validation targets evaluated: {diagnostics.CrossValidationTargetsEvaluated}");
                lines.Add(string.Format(
                    CultureInfo.InvariantCulture,
                    "Mean cross-validation error: {0:F3}",
                    diagnostics.MeanCrossValidationError));
                lines.Add(string.Format(
                    CultureInfo.InvariantCulture,
                    "Maximum cross-validation error: {0:F3}",
                    diagnostics.MaximumCrossValidationError));
            }

            if (diagnostics.RegressionFailures > 0)
                lines.Add($"Regression failures: {diagnostics.RegressionFailures}");

            if (diagnostics.MeanErrorExceeded)
                lines.Add("The mean-error threshold was exceeded.");

            if (diagnostics.MaximumErrorExceeded)
                lines.Add("The maximum-error threshold was exceeded.");

            return string.Join(Environment.NewLine, lines);
        }

        public static string FailureReason(CalibrationFailureReason reason)
        {
            return reason switch
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
                _ => "Calibration failed without a specific failure reason.",
            };
        }
    }
}
