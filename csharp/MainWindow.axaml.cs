using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Media.Imaging;
using Avalonia.Threading;
using UFeel;

namespace UFeelTest;

public partial class MainWindow : Window
{
    private readonly UFeelProcessor processor;
    private readonly CancellationTokenSource cancellation = new();
    private readonly SemaphoreSlim processorGate = new(1, 1);

    private readonly Task processTask;
    private Task? calibrationTask;
    private CancellationTokenSource? calibrationCancellation;
    private EyeTrackingResultData lastEyeTracking = new(
        EyeTrackingStatus.NotCalibrated,
        0.0F,
        0.0F,
        0.0F,
        0.0F,
        0.0F,
        EyeTrackingDirection.None);

    private const double EyeTrackingSmoothingTimeConstant = 0.08;
    private const double EyeTrackingResetGapSeconds = 0.5;

    private double? smoothedEyeTrackingX;
    private double? smoothedEyeTrackingY;
    private long? lastEyeTrackingTimestamp;

    private readonly string calibrationProfilePath;
    private float calibrationTargetX;
    private float calibrationTargetY;
    private bool speechEnabled;
    private bool closing;

    public MainWindow()
    {
        InitializeComponent();

        string? libraryPath =
            Environment.GetEnvironmentVariable("UFEEL_PACKAGE_PATH");

        if (string.IsNullOrWhiteSpace(libraryPath))
            throw new InvalidOperationException(
                "UFEEL_PACKAGE_PATH is not set.");

        libraryPath = Path.GetFullPath(libraryPath);

        string packageDirectory =
            Path.GetDirectoryName(libraryPath)
            ?? throw new InvalidOperationException(
                "Cannot determine the package directory.");

        string speechModelPath = Path.Combine(
            packageDirectory,
            "models/SpeechToText/",
            "vosk-model-small-fr-0.22");
            // "vosk-model-small-en-us-0.15");
            // "vosk-model-small-de-0.15");
        string modelDirectory = Path.Combine(packageDirectory, "models");

        if (!Directory.Exists(modelDirectory))
            throw new DirectoryNotFoundException(
                $"Model directory not found: {modelDirectory}");
        if (!Directory.Exists(speechModelPath))
            throw new DirectoryNotFoundException(
                $"Vosk model directory not found: {speechModelPath}");

        string localDataDirectory = Environment.GetFolderPath(
            Environment.SpecialFolder.LocalApplicationData);
        if (string.IsNullOrWhiteSpace(localDataDirectory))
            throw new InvalidOperationException(
                "Cannot determine the per-user application data directory for the eyeTracking profile.");

        calibrationProfilePath = Path.Combine(
            localDataDirectory,
            "UFeel",
            "eyeTracking-profile-v3.txt");

        processor = new UFeelProcessor(
            modelDirectory,
            speechModelPath,
            libraryPath);

        if (File.Exists(calibrationProfilePath))
        {
            try
            {
                StatusText.Text = processor.LoadEyeTrackingCalibrationProfile(calibrationProfilePath)
                    ? "Loaded saved eye-tracking calibration"
                    : "Saved eye-tracking calibration is invalid, recalibrate to replace it";
            }
            catch (Exception exception)
            {
                StatusText.Text = $"Could not load eye-tracking calibration: {exception.Message}";
            }
        }

        CalibrationCanvas.SizeChanged += (_, _) =>
            UpdateCalibrationMarker(calibrationTargetX, calibrationTargetY);
        EyeTrackingCanvas.SizeChanged += (_, _) =>
        {
            if (EyeTrackingOverlay.IsVisible)
                UpdateEyeTrackingMarker(lastEyeTracking);
        };
        KeyDown += MainWindow_OnKeyDown;
        Closed += OnClosed;
        processTask = ProcessLoopAsync(cancellation.Token);
    }

    private async Task ProcessLoopAsync(CancellationToken token)
    {
        while (!token.IsCancellationRequested)
        {
            Bitmap? bitmap = null;

            try
            {
                bool updated;
                Dictionary<string, float> emotions;
                EyeTrackingResultData eyeTracking;
                string speechToText;
                HeartRateResultData heartRate;

                await processorGate.WaitAsync(token);

                try
                {
                    updated = processor.Update(); // TODO: avoid grouping all features together for the update phase, update one by one and at separate timestamps

                    if (!updated)
                    {
                        emotions = [];
                        eyeTracking = new(
                            EyeTrackingStatus.FrameUnavailable,
                            0.0F,
                            0.0F,
                            0.0F,
                            0.0F,
                            0.0F,
                            EyeTrackingDirection.None);
                        smoothedEyeTrackingX = null;
                        smoothedEyeTrackingY = null;
                        lastEyeTrackingTimestamp = null;
                        speechToText = string.Empty;
                        heartRate = new HeartRateResultData(
                            HeartRateStatus.RequestFailed,
                            0);
                    }
                    else
                    {
                        FrameData? frame = processor.GetFrame();

                        if (frame is not null)
                            bitmap = FrameBitmapFactory.Create(frame);

                        emotions = processor.GetEmotions();
                        eyeTracking = SmoothEyeTracking(processor.GetEyeTracking());
                        speechToText = speechEnabled
                            ? processor.GetSpeechToText()
                            : string.Empty;
                        heartRate = processor.GetHeartRate();
                    }
                }
                finally
                {
                    processorGate.Release();
                }

                if (!updated)
                {
                    await Dispatcher.UIThread.InvokeAsync(() =>
                    {
                        if (closing)
                            return;

                        DirectionsText.Text = FormatDirections(eyeTracking);
                        EyeTrackingText.Text = FormatEyeTracking(eyeTracking: eyeTracking);
                        lastEyeTracking = eyeTracking;
                        EyeTrackingOverlay.IsVisible = false;
                    });
                    await Task.Delay(50, token);
                    continue;
                }

                Bitmap? nextBitmap = bitmap;
                bitmap = null;

                await Dispatcher.UIThread.InvokeAsync(() =>
                {
                    if (closing)
                    {
                        nextBitmap?.Dispose();
                        return;
                    }

                    if (nextBitmap is not null)
                    {
                        Bitmap? oldBitmap = FrameImage.Source as Bitmap;
                        FrameImage.Source = nextBitmap;
                        oldBitmap?.Dispose();
                    }

                    EmotionsText.Text = FormatEmotions(emotions);
                    DirectionsText.Text = FormatDirections(eyeTracking);
                    EyeTrackingText.Text = FormatEyeTracking(eyeTracking);
                    lastEyeTracking = eyeTracking;
                    EyeTrackingOverlay.IsVisible = eyeTracking.IsValid && !CalibrationOverlay.IsVisible;
                    if (EyeTrackingOverlay.IsVisible)
                        UpdateEyeTrackingMarker(eyeTracking);
                    SpeechToTextText.Text = string.IsNullOrWhiteSpace(speechToText)
                        ? "No speech detected"
                        : speechToText;
                    HeartRate.Text = FormatHeartRate(heartRate);
                });

                await Task.Delay(33, token);
            }
            catch (OperationCanceledException) when (token.IsCancellationRequested)
            {
                break;
            }
            catch (Exception exception)
            {
                bitmap?.Dispose();

                await Dispatcher.UIThread.InvokeAsync(() =>
                {
                    if (!closing)
                        StatusText.Text = exception.Message;
                });

                try
                {
                    await Task.Delay(500, token);
                }
                catch (OperationCanceledException)
                {
                    break;
                }
            }
            finally
            {
                bitmap?.Dispose();
            }
        }
    }

    private static string FormatEmotions(
        Dictionary<string, float> emotions)
    {
        if (emotions.Count == 0)
            return "No data";

        return string.Join(
            Environment.NewLine,
            emotions.OrderByDescending(pair => pair.Value)
                .Select(pair => $"{pair.Key}: {pair.Value:F3}"));
    }

    private static string FormatDirections(EyeTrackingResultData eyeTracking)
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

    private static string FormatEyeTracking(EyeTrackingResultData eyeTracking)
    {
        return eyeTracking.IsValid
            ? $"Recorded: ({eyeTracking.RecordedX:F3}, {eyeTracking.RecordedY:F3})" +
            Environment.NewLine +
            $"Estimated: ({eyeTracking.EstimatedX:F3}, {eyeTracking.EstimatedY:F3})" +
            Environment.NewLine +
            $"Confidence: {eyeTracking.Confidence:P0}"
            : "Screen position unavailable";
    }

    private EyeTrackingResultData SmoothEyeTracking(EyeTrackingResultData eyeTracking)
    {
        if (!eyeTracking.IsValid)
        {
            smoothedEyeTrackingX = null;
            smoothedEyeTrackingY = null;
            lastEyeTrackingTimestamp = null;
            return eyeTracking;
        }

        long timestamp = Stopwatch.GetTimestamp();

        double dt = lastEyeTrackingTimestamp is null
            ? 0.0
            : (timestamp - lastEyeTrackingTimestamp.Value) /
                (double)Stopwatch.Frequency;

        lastEyeTrackingTimestamp = timestamp;

        if (smoothedEyeTrackingX is null ||
            smoothedEyeTrackingY is null ||
            dt <= 0.0 ||
            dt > EyeTrackingResetGapSeconds)
        {
            smoothedEyeTrackingX = eyeTracking.EstimatedX;
            smoothedEyeTrackingY = eyeTracking.EstimatedY;
            return eyeTracking;
        }

        double alpha =
            1.0 - Math.Exp(-dt / EyeTrackingSmoothingTimeConstant);

        smoothedEyeTrackingX += alpha * (eyeTracking.EstimatedX - smoothedEyeTrackingX.Value);
        smoothedEyeTrackingY += alpha * (eyeTracking.EstimatedY - smoothedEyeTrackingY.Value);

        return eyeTracking with
        {
            EstimatedX = (float)smoothedEyeTrackingX.Value,
            EstimatedY = (float)smoothedEyeTrackingY.Value
        };
    }

    private void UpdateEyeTrackingMarker(EyeTrackingResultData eyeTracking)
    {
        double width = EyeTrackingCanvas.Bounds.Width;
        double height = EyeTrackingCanvas.Bounds.Height;
        if (width <= 0 || height <= 0)
            return;

        Canvas.SetLeft(
            EyeTrackingMarker,
            eyeTracking.EstimatedX * width - EyeTrackingMarker.Width / 2);

        Canvas.SetTop(
            EyeTrackingMarker,
            eyeTracking.EstimatedY * height - EyeTrackingMarker.Height / 2);
    }

    private static string FormatHeartRate(HeartRateResultData heartRate)
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

    private async void SpeechToTextButton_OnClick(
        object? sender,
        RoutedEventArgs e)
    {
        bool requestedState = !speechEnabled;

        try
        {
            await processorGate.WaitAsync(cancellation.Token);

            try
            {
                processor.SetSpeechToTextEnabled(requestedState);
                speechEnabled = requestedState;
            }
            finally
            {
                processorGate.Release();
            }

            SpeechToTextButton.Content = speechEnabled
                ? "Disable speech"
                : "Enable speech";
        }
        catch (OperationCanceledException)
        {
        }
        catch (Exception exception)
        {
            StatusText.Text = exception.Message;
        }
    }

    private async void CalibrateButton_OnClick(
        object? sender,
        RoutedEventArgs e)
    {
        if (calibrationCancellation is not null)
        {
            calibrationCancellation.Cancel();
            return;
        }

        var source = CancellationTokenSource.CreateLinkedTokenSource(
            cancellation.Token);
        calibrationCancellation = source;
        CalibrateButton.Content = "Cancel calibration";

        try
        {
            calibrationTask = RunCalibrationAsync(source.Token);
            await calibrationTask;
        }
        finally
        {
            calibrationTask = null;
            calibrationCancellation = null;
            source.Dispose();
            CalibrateButton.Content = "Calibrate / recalibrate";
        }
    }

    private async Task RunCalibrationAsync(CancellationToken token)
    {
        CalibrationDiagnosticsText.Text = "No calibration diagnostics yet.";

        bool calibrationStarted = false;

        try
        {
            await processorGate.WaitAsync(token);
            try
            {
                processor.BeginEyeTrackingCalibration();
                calibrationStarted = true;
            }
            finally
            {
                processorGate.Release();
            }

            CalibrationOverlay.IsVisible = true;
            await WaitForCalibrationReadinessAsync(token);

            (float X, float Y)[] targets =
            [
                (0.5F, 0.5F),
                (0.1F, 0.1F),
                (0.1F, 0.9F),
                (0.9F, 0.1F),
                (0.9F, 0.9F),
                (0.1F, 0.5F),
                (0.5F, 0.1F),
                (0.9F, 0.5F),
                (0.5F, 0.9F)
            ];

            for (int i = 0; i < targets.Length; i++)
            {
                var target = targets[i];
                calibrationTargetX = target.X;
                calibrationTargetY = target.Y;
                UpdateCalibrationMarker(target.X, target.Y);
                CalibrationInstruction.Text =
                    $"Look at the target and keep your head still ({i + 1}/{targets.Length})";

                await Task.Delay(TimeSpan.FromSeconds(1), token);
                await processorGate.WaitAsync(token);
                try
                {
                    processor.SetEyeTrackingCalibrationTarget(target.X, target.Y);
                }
                finally
                {
                    processorGate.Release();
                }

                Stopwatch targetTimer = Stopwatch.StartNew();
                EyeTrackingCalibrationProgress progress;
                do
                {
                    await processorGate.WaitAsync(token);
                    try
                    {
                        progress = processor.GetEyeTrackingCalibrationProgress();
                    }
                    finally
                    {
                        processorGate.Release();
                    }

                    StatusText.Text =
                        $"Calibration target {i + 1}/{targets.Length}: " +
                        $"{progress.SamplesForCurrentTarget}/{progress.RequiredSamplesPerTarget} usable frames";
                    CalibrationInstruction.Text =
                        $"Keep looking at the target ({progress.SamplesForCurrentTarget}/" +
                        $"{progress.RequiredSamplesPerTarget} usable frames)";
                    if ((progress.SamplesForCurrentTarget >= progress.RequiredSamplesPerTarget &&
                         targetTimer.Elapsed >= TimeSpan.FromSeconds(1)) ||
                        targetTimer.Elapsed >= TimeSpan.FromSeconds(3))
                    {
                        break;
                    }
                    await Task.Delay(200, token);
                }
                while (true);
            }

            await processorGate.WaitAsync(token);

            bool calibrated;
            bool profileSaved = false;
            EyeTrackingCalibrationDiagnostics? diagnostics = null;

            try
            {
                calibrated = processor.FinishEyeTrackingCalibration();
                calibrationStarted = false;

                if (calibrated)
                {
                    profileSaved = SaveEyeTrackingCalibrationProfile();
                    CalibrationDiagnosticsText.Text =
                        "Calibration completed successfully.";
                }
                else
                {
                    diagnostics = processor.GetEyeTrackingCalibrationDiagnostics();

                    string report = FormatCalibrationFailure(diagnostics);
                    CalibrationDiagnosticsText.Text = report;
                    StatusText.Text = report;
                }
            }
            finally
            {
                processorGate.Release();
            }

            if (calibrated)
            {
                StatusText.Text = profileSaved
                    ? "Eye tracking calibrated and profile saved"
                    : "Eye tracking calibrated, but the profile could not be saved";
            }
        }
        catch (OperationCanceledException) when (token.IsCancellationRequested)
        {
            if (!closing)
                StatusText.Text = "Eye tracking calibration cancelled";
        }
        catch (Exception exception)
        {
            if (!closing)
                StatusText.Text = exception.Message;
        }
        finally
        {
            try
            {
                if (calibrationStarted)
                {
                    try
                    {
                        await processorGate.WaitAsync();
                        try
                        {
                            processor.CancelEyeTrackingCalibration();
                        }
                        finally
                        {
                            processorGate.Release();
                        }
                    }
                    catch (Exception exception)
                    {
                        if (!closing)
                        {
                            StatusText.Text =
                                $"Could not cancel eye-tracking calibration: {exception.Message}";
                        }
                    }
                }
            }
            finally
            {
                CalibrationOverlay.IsVisible = false;
                EyeTrackingOverlay.IsVisible = lastEyeTracking.IsValid;
                if (EyeTrackingOverlay.IsVisible)
                    UpdateEyeTrackingMarker(lastEyeTracking);
            }
        }
    }

    private async Task WaitForCalibrationReadinessAsync(CancellationToken token)
    {
        Stopwatch? readyTimer = null;

        while (true)
        {
            token.ThrowIfCancellationRequested();

            if (lastEyeTracking.Status == EyeTrackingStatus.Calibrating)
            {
                readyTimer ??= Stopwatch.StartNew();
                CalibrationInstruction.Text =
                    "Face detected. Keep your face steady and eyes open";
                StatusText.Text = "Hold a steady position before calibration";
                if (readyTimer.Elapsed >= TimeSpan.FromSeconds(2))
                    return;
            }
            else
            {
                readyTimer = null;
                CalibrationInstruction.Text =
                    "Position your face in the camera view and keep your eyes open";
                StatusText.Text = "Waiting for a valid face and eye landmarks";
            }

            await Task.Delay(100, token);
        }
    }

    private bool SaveEyeTrackingCalibrationProfile()
    {
        string? profileDirectory = Path.GetDirectoryName(calibrationProfilePath);

        if (string.IsNullOrWhiteSpace(profileDirectory))
            throw new InvalidOperationException(
                "Cannot determine the eye-tracking profile directory.");

        Directory.CreateDirectory(profileDirectory);

        return processor.SaveEyeTrackingCalibrationProfile(calibrationProfilePath);
    }

    private void UpdateCalibrationMarker(float x, float y)
    {
        double width = CalibrationCanvas.Bounds.Width;
        double height = CalibrationCanvas.Bounds.Height;
        if (width <= 0 || height <= 0)
            return;

        Canvas.SetLeft(CalibrationMarker, x * width - CalibrationMarker.Width / 2);
        Canvas.SetTop(CalibrationMarker, y * height - CalibrationMarker.Height / 2);
    }

    private static string FormatCalibrationFailure(
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

    private void MainWindow_OnKeyDown(object? sender, KeyEventArgs e)
    {
        if (e.Key == Key.Escape && calibrationCancellation is not null)
        {
            calibrationCancellation.Cancel();
            e.Handled = true;
        }
    }

    private async void OnClosed(object? sender, EventArgs e)
    {
        closing = true;
        cancellation.Cancel();
        calibrationCancellation?.Cancel();

        await processTask;
        if (calibrationTask is not null)
            await calibrationTask;

        try
        {
            await processorGate.WaitAsync();

            try
            {
                processor.SetSpeechToTextEnabled(false);
                processor.Dispose();
            }
            finally
            {
                processorGate.Release();
            }
        }
        finally
        {
            (FrameImage.Source as Bitmap)?.Dispose();
            FrameImage.Source = null;

            processorGate.Dispose();
            cancellation.Dispose();
        }
    }
}
