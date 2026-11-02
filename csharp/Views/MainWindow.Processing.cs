using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Threading;
using System.Threading.Tasks;
using Avalonia.Controls;
using Avalonia.Media.Imaging;
using Avalonia.Threading;
using UFeel;

namespace UFeelTest;

public partial class MainWindow
{
    private static readonly EyeTrackingResultData NotCalibratedEyeTracking = new(
        EyeTrackingStatus.NotCalibrated,
        0.0F,
        0.0F,
        0.0F,
        0.0F,
        0.0F,
        EyeTrackingDirection.None);

    private static readonly EyeTrackingResultData FrameUnavailableEyeTracking = new(
        EyeTrackingStatus.FrameUnavailable,
        0.0F,
        0.0F,
        0.0F,
        0.0F,
        0.0F,
        EyeTrackingDirection.None);

    private static readonly HeartRateResultData RequestFailedHeartRate = new(
        HeartRateStatus.RequestFailed,
        0);

    private async Task ProcessLoopAsync(CancellationToken token)
    {
        Dictionary<string, float> emotions = [];
        EyeTrackingResultData eyeTracking = FrameUnavailableEyeTracking;
        string speechToText = string.Empty;
        HeartRateResultData heartRate = RequestFailedHeartRate;

        while (!token.IsCancellationRequested)
        {
            Bitmap? bitmap = null;

            try
            {
                bool updated;
                DetectionType due = scheduler.GetDue(Stopwatch.GetTimestamp());

                await processorGate.WaitAsync(token);

                try
                {
                    processor.UpdateFrame();
                    updated = due == DetectionType.None || processor.Update(due);

                    if (!updated)
                    {
                        emotions = [];
                        eyeTracking = FrameUnavailableEyeTracking;
                        eyeTrackingSmoother.Reset();
                        speechToText = string.Empty;
                        heartRate = RequestFailedHeartRate;
                    }
                    else
                    {
                        bitmap = frameBitmaps.Update(processor);

                        if (due.HasFlag(DetectionType.Emotions))
                            emotions = processor.GetEmotions();

                        if (due.HasFlag(DetectionType.EyeTracking))
                            eyeTracking = eyeTrackingSmoother.Smooth(processor.GetEyeTracking());

                        if (due.HasFlag(DetectionType.SpeechToText))
                            speechToText = speechEnabled
                                ? processor.GetSpeechToText()
                                : string.Empty;

                        if (due.HasFlag(DetectionType.HeartRate))
                            heartRate = processor.GetHeartRate();
                    }
                }
                finally
                {
                    processorGate.Release();
                }

                if (!updated)
                {
                    EyeTrackingResultData unavailable = eyeTracking;
                    await Dispatcher.UIThread.InvokeAsync(() => ShowFrameUnavailable(unavailable));
                    await Task.Delay(50, token);
                    continue;
                }

                Dictionary<string, float> shownEmotions = emotions;
                EyeTrackingResultData shownEyeTracking = eyeTracking;
                string shownSpeech = speechToText;
                HeartRateResultData shownHeartRate = heartRate;

                await Dispatcher.UIThread.InvokeAsync(() =>
                    ShowResults(
                        bitmap,
                        shownEmotions,
                        shownEyeTracking,
                        shownSpeech,
                        shownHeartRate));

                await Task.Delay(33, token);
            }
            catch (OperationCanceledException) when (token.IsCancellationRequested)
            {
                break;
            }
            catch (Exception exception)
            {
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
        }
    }

    private void ShowFrameUnavailable(EyeTrackingResultData eyeTracking)
    {
        if (closing)
            return;

        DirectionsText.Text = DisplayFormatter.FormatDirections(eyeTracking);
        EyeTrackingText.Text = DisplayFormatter.FormatEyeTracking(eyeTracking);
        lastEyeTracking = eyeTracking;
        EyeTrackingOverlay.IsVisible = false;
    }

    private void ShowResults(
        Bitmap? bitmap,
        Dictionary<string, float> emotions,
        EyeTrackingResultData eyeTracking,
        string speechToText,
        HeartRateResultData heartRate)
    {
        if (closing)
            return;

        if (bitmap is not null)
            FrameImage.Source = bitmap;

        EmotionsText.Text = DisplayFormatter.FormatEmotions(emotions);
        DirectionsText.Text = DisplayFormatter.FormatDirections(eyeTracking);
        EyeTrackingText.Text = DisplayFormatter.FormatEyeTracking(eyeTracking);
        lastEyeTracking = eyeTracking;
        EyeTrackingOverlay.IsVisible = eyeTracking.IsValid && !CalibrationOverlay.IsVisible;
        if (EyeTrackingOverlay.IsVisible)
            UpdateEyeTrackingMarker(eyeTracking);
        SpeechToTextText.Text = string.IsNullOrWhiteSpace(speechToText)
            ? "No speech detected"
            : speechToText;
        HeartRateText.Text = DisplayFormatter.FormatHeartRate(heartRate);
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
}
