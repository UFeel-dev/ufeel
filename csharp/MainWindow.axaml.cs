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
    private GazeResultData lastGaze = new(
        0.0F,
        0.0F,
        0.0F,
        0.0F,
        0.0F,
        UFeelNative.GazeDirection.None,
        UFeelNative.GazeStatus.NotCalibrated);
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
            libraryPath,
            modelDirectory,
            speechModelPath);

        if (File.Exists(calibrationProfilePath))
        {
            try
            {
                StatusText.Text = processor.LoadCalibrationProfile(calibrationProfilePath)
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
                HeartRateResultData heartrate;

                await processorGate.WaitAsync(token);

                try
                {
                    updated = processor.Update();

                    if (!updated)
                    {
                        emotions = new();
                        gaze = new(
                            0.0F,
                            0.0F,
                            0.0F,
                            0.0F,
                            0.0F,
                            UFeelNative.GazeDirection.None,
                            UFeelNative.GazeStatus.FrameUnavailable);
                        speech = string.Empty;
                        heartrate = new HeartRateResultData(
                            HeartRateStatus.RequestFailed,
                            0);
                    }
                    else
                    {
                        FrameData? frame = processor.GetFrame();

                        if (frame is not null)
                            bitmap = FrameBitmapFactory.Create(frame);

                        emotions = processor.GetEmotions();
                        gaze = processor.GetGazeResult();
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
                    HeartRate.Text = FormatHeartRate(heartrate);
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
                UFeelNative.GazeStatus.NotCalibrated => "Not calibrated",
                UFeelNative.GazeStatus.FaceUnavailable => "Face not detected",
                UFeelNative.GazeStatus.Blinking => "Blink detected",
                UFeelNative.GazeStatus.LowQuality => "Eye landmarks are low quality",
                UFeelNative.GazeStatus.Calibrating => "Calibration in progress",
                UFeelNative.GazeStatus.Disabled => "Eye tracking is disabled",
                UFeelNative.GazeStatus.FrameUnavailable => "Camera frame unavailable",
                _ => "Gaze unavailable"
            };
        }

        var directions = new List<string>();
        if (gaze.Directions.HasFlag(UFeelNative.GazeDirection.Left))
            directions.Add("left");
        if (gaze.Directions.HasFlag(UFeelNative.GazeDirection.Right))
            directions.Add("right");
        if (gaze.Directions.HasFlag(UFeelNative.GazeDirection.Up))
            directions.Add("up");
        if (gaze.Directions.HasFlag(UFeelNative.GazeDirection.Down))
            directions.Add("down");
        if (gaze.Directions.HasFlag(UFeelNative.GazeDirection.Center))
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

    private static string FormatHeartRate(HeartRateResultData heartrate)
    {
        if (!heartrate.IsValid)
        {
            return heartrate.Status switch
            {
                HeartRateStatus.JsonParseError => "JSON parse error",
                HeartRateStatus.JsonParseOutOfRange => "JSON parse - out of range",
                HeartRateStatus.RequestFailed => "Request failed",
                _ => "Heartrate unavailable"
            };
        }

        return heartrate.Bpm.ToString();
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
        bool calibrationStarted = false;

        try
        {
            await processorGate.WaitAsync(token);
            try
            {
                processor.BeginCalibration();
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
                    processor.SetCalibrationTarget(target.X, target.Y);
                }
                finally
                {
                    processorGate.Release();
                }

                Stopwatch targetTimer = Stopwatch.StartNew();
                CalibrationProgress progress;
                do
                {
                    await processorGate.WaitAsync(token);
                    try
                    {
                        progress = processor.GetCalibrationProgress();
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
            CalibrationProgress? failureProgress = null;
            try
            {
                calibrated = processor.FinishCalibration();
                calibrationStarted = false;
                if (calibrated)
                    profileSaved = SaveCalibrationProfile();
                else
                    failureProgress = processor.GetCalibrationProgress();
            }
            finally
            {
                processorGate.Release();
            }

            StatusText.Text = calibrated
                ? profileSaved
                    ? "Eye tracking calibrated and profile saved"
                    : "Eye tracking calibrated, but the profile could not be saved"
                : failureProgress is not null &&
                    failureProgress.TargetsWithEnoughSamples < targets.Length
                    ? $"Calibration incomplete: only {failureProgress.TargetsWithEnoughSamples}/" +
                        $"{targets.Length} targets had enough usable frames"
                    : "Calibration quality check failed, keep your face steady and use widely spaced targets";
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
