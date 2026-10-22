using System.Diagnostics;
using Avalonia;
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
    private GazeResultData lastGaze = new(
        GazeStatus.NotCalibrated,
        0.0F,
        0.0F,
        0.0F,
        0.0F,
        0.0F,
        GazeDirection.None);

    private const double GazeSmoothingTimeConstant = 0.08;
    private const double GazeResetGapSeconds = 0.5;

    private double? smoothedGazeX;
    private double? smoothedGazeY;
    private long? lastGazeTimestamp;

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
            "models",
            "vosk-model-small-fr-0.22");
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
                "Cannot determine the per-user application data directory for the gaze profile.");

        calibrationProfilePath = Path.Combine(
            localDataDirectory,
            "UFeel",
            "gaze-profile-v3.txt");

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
        GazeCanvas.SizeChanged += (_, _) =>
        {
            if (GazeOverlay.IsVisible)
                UpdateGazeMarker(lastGaze);
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
                GazeResultData gaze;
                string speech;
                uint heartrate;

                await processorGate.WaitAsync(token);

                try
                {
                    updated = processor.Update();

                    if (!updated)
                    {
                        emotions = [];
                        gaze = new(
                            GazeStatus.FrameUnavailable,
                            0.0F,
                            0.0F,
                            0.0F,
                            0.0F,
                            0.0F,
                            GazeDirection.None);
                        smoothedGazeX = null;
                        smoothedGazeY = null;
                        lastGazeTimestamp = null;
                        speech = string.Empty;
                        heartrate = 0;
                    }
                    else
                    {
                        FrameData? frame = processor.GetFrame();

                        if (frame is not null)
                            bitmap = FrameBitmapFactory.Create(frame);

                        emotions = processor.GetEmotions();
                        gaze = SmoothGaze(processor.GetGaze());
                        speech = speechEnabled
                            ? processor.GetSpeechToText()
                            : string.Empty;
                        heartrate = processor.GetHeartRate();
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

                        DirectionsText.Text = FormatDirections(gaze);
                        GazeText.Text = FormatGaze(gaze: gaze);
                        lastGaze = gaze;
                        GazeOverlay.IsVisible = false;
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
                    DirectionsText.Text = FormatDirections(gaze);
                    GazeText.Text = FormatGaze(gaze);
                    lastGaze = gaze;
                    GazeOverlay.IsVisible = gaze.IsValid && !CalibrationOverlay.IsVisible;
                    if (GazeOverlay.IsVisible)
                        UpdateGazeMarker(gaze);
                    SpeechText.Text = string.IsNullOrWhiteSpace(speech)
                        ? "No speech detected"
                        : speech;
                    HeartRate.Text = heartrate.ToString();
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

    private static string FormatDirections(GazeResultData gaze)
    {
        if (!gaze.IsValid)
        {
            return gaze.Status switch
            {
                GazeStatus.NotCalibrated => "Not calibrated",
                GazeStatus.FaceUnavailable => "Face not detected",
                GazeStatus.Blinking => "Blink detected",
                GazeStatus.LowQuality => "Eye landmarks are low quality",
                GazeStatus.Calibrating => "Calibration in progress",
                GazeStatus.Disabled => "Eye tracking is disabled",
                GazeStatus.FrameUnavailable => "Camera frame unavailable",
                _ => "Gaze unavailable"
            };
        }

        var directions = new List<string>();
        if (gaze.Directions.HasFlag(GazeDirection.Left))
            directions.Add("left");
        if (gaze.Directions.HasFlag(GazeDirection.Right))
            directions.Add("right");
        if (gaze.Directions.HasFlag(GazeDirection.Up))
            directions.Add("up");
        if (gaze.Directions.HasFlag(GazeDirection.Down))
            directions.Add("down");
        if (gaze.Directions.HasFlag(GazeDirection.Center))
            directions.Add("center");

        return directions.Count == 0
            ? "No direction"
            : string.Join(", ", directions);
    }

    private static string FormatGaze(GazeResultData gaze)
    {
        return gaze.IsValid
            ? $"Recorded: ({gaze.RecordedX:F3}, {gaze.RecordedY:F3})" +
            Environment.NewLine +
            $"Estimated: ({gaze.EstimatedX:F3}, {gaze.EstimatedY:F3})" +
            Environment.NewLine +
            $"Confidence: {gaze.Confidence:P0}"
            : "Screen position unavailable";
    }

    private GazeResultData SmoothGaze(GazeResultData gaze)
    {
        if (!gaze.IsValid)
        {
            smoothedGazeX = null;
            smoothedGazeY = null;
            lastGazeTimestamp = null;
            return gaze;
        }

        long timestamp = Stopwatch.GetTimestamp();

        double dt = lastGazeTimestamp is null
            ? 0.0
            : (timestamp - lastGazeTimestamp.Value) /
                (double)Stopwatch.Frequency;

        lastGazeTimestamp = timestamp;

        if (smoothedGazeX is null ||
            smoothedGazeY is null ||
            dt <= 0.0 ||
            dt > GazeResetGapSeconds)
        {
            smoothedGazeX = gaze.EstimatedX;
            smoothedGazeY = gaze.EstimatedY;
            return gaze;
        }

        double alpha =
            1.0 - Math.Exp(-dt / GazeSmoothingTimeConstant);

        smoothedGazeX += alpha * (gaze.EstimatedX - smoothedGazeX.Value);
        smoothedGazeY += alpha * (gaze.EstimatedY - smoothedGazeY.Value);

        return gaze with
        {
            EstimatedX = (float)smoothedGazeX.Value,
            EstimatedY = (float)smoothedGazeY.Value
        };
    }

    private void UpdateGazeMarker(GazeResultData gaze)
    {
        double width = GazeCanvas.Bounds.Width;
        double height = GazeCanvas.Bounds.Height;
        if (width <= 0 || height <= 0)
            return;

        Canvas.SetLeft(
            GazeMarker,
            gaze.EstimatedX * width - GazeMarker.Width / 2);

        Canvas.SetTop(
            GazeMarker,
            gaze.EstimatedY * height - GazeMarker.Height / 2);
    }

    private async void SpeechButton_OnClick(
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

            SpeechButton.Content = speechEnabled
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
                GazeOverlay.IsVisible = lastGaze.IsValid;
                if (GazeOverlay.IsVisible)
                    UpdateGazeMarker(lastGaze);
            }
        }
    }

    private async Task WaitForCalibrationReadinessAsync(CancellationToken token)
    {
        Stopwatch? readyTimer = null;

        while (true)
        {
            token.ThrowIfCancellationRequested();

            if (lastGaze.Status == GazeStatus.Calibrating)
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
