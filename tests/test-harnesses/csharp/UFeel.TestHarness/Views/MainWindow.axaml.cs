using System;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Threading;
using UFeel;

namespace UFeelTest;

public partial class MainWindow : Window
{
    private readonly CalibrationProfileStore profiles = CalibrationProfileStore.CreateDefault();
    private readonly FrameBitmapBuffer frameBitmaps;
    private readonly UFeelSession session;

    private string idleStatus = string.Empty;
    private bool closing;

    public MainWindow()
    {
        InitializeComponent();

        HarnessPaths paths = HarnessPaths.FromEnvironment();
        NativeLibraryLoader.Use(paths.LibraryPath);

        var processor = new UFeelProcessor(paths.Layout);
        ShowIdleStatus(LoadProfile(processor));

        var demand = FeatureDemand.CreateDefault();
        demand.Pin(DetectionTypes.All & ~DetectionType.SpeechToText);

        frameBitmaps = new FrameBitmapBuffer(bitmap =>
        {
            if (!closing)
                FrameImage.Source = bitmap;
        });

        session = new UFeelSession(processor, demand: demand, frameObserver: frameBitmaps);
        session.SnapshotPublished += snapshot => Dispatcher.UIThread.Post(() => ShowSnapshot(snapshot));

        CalibrationCanvas.SizeChanged += (_, _) => UpdateCalibrationMarker();
        EyeTrackingCanvas.SizeChanged += (_, _) => UpdateEyeTrackingMarker();
        KeyDown += MainWindow_OnKeyDown;
        Closed += OnClosed;

        session.Start();
    }

    private string LoadProfile(UFeelProcessor processor)
    {
        try
        {
            return profiles.Load(processor) switch
            {
                ProfileLoadResult.Loaded => "Loaded saved eye-tracking calibration",
                ProfileLoadResult.Invalid => "Saved eye-tracking calibration is invalid, recalibrate to replace it",
                _ => string.Empty,
            };
        }
        catch (Exception exception)
        {
            return $"Could not load eye-tracking calibration: {exception.Message}";
        }
    }

    private async void SpeechToTextButton_OnClick(object? sender, RoutedEventArgs e)
    {
        bool requested = !session.IsSpeechToTextEnabled;

        try
        {
            await session.SetSpeechToTextEnabledAsync(requested);
            SpeechToTextButton.Content = requested ? "Disable speech" : "Enable speech";
        }
        catch (OperationCanceledException)
        {
        }
        catch (Exception exception)
        {
            ShowIdleStatus(exception.Message);
        }
    }

    private void ShowIdleStatus(string status)
    {
        idleStatus = status;
        StatusText.Text = status;
    }

    private void OnClosed(object? sender, EventArgs e)
    {
        closing = true;
        calibrationCancellation?.Cancel();

        session.Dispose();

        FrameImage.Source = null;
        frameBitmaps.Dispose();
    }
}
