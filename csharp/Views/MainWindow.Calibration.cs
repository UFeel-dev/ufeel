using System;
using System.Diagnostics;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using UFeel;

namespace UFeelTest;

public partial class MainWindow
{
    private static readonly (float X, float Y)[] CalibrationTargets =
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

    private Task? calibrationTask;
    private CancellationTokenSource? calibrationCancellation;
    private float calibrationTargetX;
    private float calibrationTargetY;
    private bool calibrationActive;

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

    private void MainWindow_OnKeyDown(object? sender, KeyEventArgs e)
    {
        if (e.Key == Key.Escape && calibrationCancellation is not null)
        {
            calibrationCancellation.Cancel();
            e.Handled = true;
        }
    }

    private async Task RunCalibrationAsync(CancellationToken token)
    {
        CalibrationDiagnosticsText.Text = "No calibration diagnostics yet.";

        // ! calibration collects one sample per eye-tracking update, so it must not be throttled.
        scheduler.SetOverride(DetectionType.EyeTracking, TimeSpan.Zero);

        try
        {
            await processorGate.WaitAsync(token);
            try
            {
                processor.BeginEyeTrackingCalibration();
                calibrationActive = true;
            }
            finally
            {
                processorGate.Release();
            }

            CalibrationOverlay.IsVisible = true;
            await WaitForCalibrationReadinessAsync(token);

            for (int i = 0; i < CalibrationTargets.Length; i++)
                await CollectTargetAsync(i, token);

            await FinishCalibrationAsync(token);
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
            scheduler.ClearOverride(DetectionType.EyeTracking);

            try
            {
                if (calibrationActive)
                    await CancelCalibrationAsync();
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

    private async Task CollectTargetAsync(int index, CancellationToken token)
    {
        var target = CalibrationTargets[index];
        calibrationTargetX = target.X;
        calibrationTargetY = target.Y;
        UpdateCalibrationMarker(target.X, target.Y);
        CalibrationInstruction.Text =
            $"Look at the target and keep your head still ({index + 1}/{CalibrationTargets.Length})";

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
                $"Calibration target {index + 1}/{CalibrationTargets.Length}: " +
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

    private async Task FinishCalibrationAsync(CancellationToken token)
    {
        await processorGate.WaitAsync(token);

        bool calibrated;
        bool profileSaved = false;

        try
        {
            calibrated = processor.FinishEyeTrackingCalibration();
            calibrationActive = false;

            if (calibrated)
            {
                profileSaved = SaveEyeTrackingCalibrationProfile();
                CalibrationDiagnosticsText.Text =
                    "Calibration completed successfully.";
            }
            else
            {
                EyeTrackingCalibrationDiagnostics diagnostics =
                    processor.GetEyeTrackingCalibrationDiagnostics();

                string report = DisplayFormatter.FormatCalibrationFailure(diagnostics);
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

    private async Task CancelCalibrationAsync()
    {
        try
        {
            await processorGate.WaitAsync();
            try
            {
                processor.CancelEyeTrackingCalibration();
                calibrationActive = false;
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
}
