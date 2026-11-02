using System;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Media.Imaging;
using UFeel;

namespace UFeelTest;

public partial class MainWindow : Window
{
    private readonly UFeelProcessor processor;
    private readonly CancellationTokenSource cancellation = new();
    private readonly SemaphoreSlim processorGate = new(1, 1);
    private readonly FeatureScheduler scheduler = FeatureScheduler.CreateDefault();
    private readonly EyeTrackingSmoother eyeTrackingSmoother = new();

    private readonly Task processTask;
    private readonly string calibrationProfilePath;

    private EyeTrackingResultData lastEyeTracking = NotCalibratedEyeTracking;
    private volatile bool speechEnabled;
    private volatile bool closing;

    public MainWindow()
    {
        InitializeComponent();

        UFeelPaths paths = UFeelPaths.Resolve();
        calibrationProfilePath = paths.CalibrationProfilePath;

        processor = new UFeelProcessor(
            paths.ModelDirectory,
            paths.SpeechModelPath,
            paths.LibraryPath);

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
        processTask = Task.Run(() => ProcessLoopAsync(cancellation.Token));
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
