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
        EmotionResultData emotions = new(
            EmotionStatus.FrameUnavailable,
            new Dictionary<string, float>());
        EyeTrackingResultData eyeTracking = FrameUnavailableEyeTracking;
        SpeechToTextResultData speechToText = new(
            SpeechToTextStatus.Disabled,
            string.Empty);
        HeartRateResultData heartRate = RequestFailedHeartRate;

        while (!token.IsCancellationRequested)
        {
            try
            {
                UFeelStatus captureStatus;
                Bitmap? bitmap = null;
                FrameData? frame = null;
                DetectionType due = DetectionType.None;

                await processorGate.WaitAsync(token);

                try
                {
                    captureStatus = frameCapture.Capture(processor);

                    if (captureStatus == UFeelStatus.Ok)
                    {
                        bitmap = frameBitmaps.Update(processor);
                        due = scheduler.GetDue(Stopwatch.GetTimestamp());

                        if (due != DetectionType.None)
                            frame = processor.GetFrame();
                    }
                }
                finally
                {
                    processorGate.Release();
                }

                if (captureStatus != UFeelStatus.Ok)
                {
                    if (captureStatus != UFeelStatus.InvalidFrame)
                        throw new InvalidOperationException(
                            $"Camera capture failed with status {captureStatus}.");

                    emotions = new EmotionResultData(
                        EmotionStatus.FrameUnavailable,
                        new Dictionary<string, float>());
                    eyeTracking = FrameUnavailableEyeTracking;
                    eyeTrackingSmoother.Reset();
                    speechToText = new SpeechToTextResultData(
                        SpeechToTextStatus.NoSpeech,
                        string.Empty);
                    heartRate = RequestFailedHeartRate;

                    EyeTrackingResultData unavailable = eyeTracking;
                    await Dispatcher.UIThread.InvokeAsync(() => ShowFrameUnavailable(unavailable));
                    await Task.Delay(50, token);
                    continue;
                }

                if (bitmap is not null)
                    await Dispatcher.UIThread.InvokeAsync(() => ShowFrame(bitmap));

                if (frame is null)
                    continue;

                UFeelStatus detectionStatus;

                await processorGate.WaitAsync(token);

                try
                {
                    detectionStatus = processor.UpdateFromFrame(due, frame);

                    if (detectionStatus == UFeelStatus.Ok)
                    {
                        if (due.HasFlag(DetectionType.Emotions))
                            emotions = processor.GetEmotions();

                        if (due.HasFlag(DetectionType.EyeTracking))
                            eyeTracking = eyeTrackingSmoother.Smooth(processor.GetEyeTracking());

                        if (due.HasFlag(DetectionType.SpeechToText))
                            speechToText = speechEnabled
                                ? processor.GetSpeechToText()
                                : new SpeechToTextResultData(
                                    SpeechToTextStatus.Disabled,
                                    string.Empty);

                        if (due.HasFlag(DetectionType.HeartRate))
                            heartRate = processor.GetHeartRate();
                    }
                }
                finally
                {
                    processorGate.Release();
                }
                string status = detectionStatus == UFeelStatus.Ok
                    ? string.Empty
                    : $"Detection failed ({detectionStatus}), retrying on the next frame";
                EmotionResultData shownEmotions = emotions;
                EyeTrackingResultData shownEyeTracking = eyeTracking;
                SpeechToTextResultData shownSpeech = speechToText;
                HeartRateResultData shownHeartRate = heartRate;

                await Dispatcher.UIThread.InvokeAsync(() =>
                    ShowResults( status,
                        shownEmotions,
                        shownEyeTracking,
                        shownSpeech,
                        shownHeartRate));
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
        StatusText.Text = "Camera frame unavailable";
        lastEyeTracking = eyeTracking;
        EyeTrackingOverlay.IsVisible = false;
    }

    private void ShowFrame(Bitmap bitmap)
    {
        if (!closing)
            FrameImage.Source = bitmap;
    }

    private void ShowResults(
        string status,
        EmotionResultData emotions,
        EyeTrackingResultData eyeTracking,
        SpeechToTextResultData speechToText,
        HeartRateResultData heartRate)
    {
        if (closing)
            return;

        StatusText.Text = status;
        EmotionsText.Text = DisplayFormatter.FormatEmotions(emotions);
        DirectionsText.Text = DisplayFormatter.FormatDirections(eyeTracking);
        EyeTrackingText.Text = DisplayFormatter.FormatEyeTracking(eyeTracking);
        lastEyeTracking = eyeTracking;
        EyeTrackingOverlay.IsVisible = eyeTracking.IsValid && !CalibrationOverlay.IsVisible;
        if (EyeTrackingOverlay.IsVisible)
            UpdateEyeTrackingMarker(eyeTracking);
        SpeechToTextText.Text = DisplayFormatter.FormatSpeech(speechToText);
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
