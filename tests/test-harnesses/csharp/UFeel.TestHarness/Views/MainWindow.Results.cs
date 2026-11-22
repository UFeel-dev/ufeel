using System;
using Avalonia.Controls;
using UFeel;

namespace UFeelTest;

public partial class MainWindow
{
    private EyeTrackingResult lastEyeTracking = UFeelSnapshot.Initial.EyeTracking;

    private void ShowSnapshot(UFeelSnapshot snapshot)
    {
        if (closing)
            return;

        if (!CalibrationOverlay.IsVisible)
            StatusText.Text = snapshot.Message ?? idleStatus;

        EmotionsText.Text = snapshot.Emotions.IsValid
            ? $"Dominant: {UFeelText.DominantEmotion(snapshot.Emotions)}{Environment.NewLine}{UFeelText.Emotions(snapshot.Emotions)}"
            : UFeelText.Emotions(snapshot.Emotions);

        lastEyeTracking = snapshot.EyeTracking;
        DirectionsText.Text = lastEyeTracking.IsTracked
            ? $"{UFeelText.Directions(lastEyeTracking)}{Environment.NewLine}{UFeelText.Eyes(lastEyeTracking)}"
            : UFeelText.Directions(lastEyeTracking);
        EyeTrackingText.Text = UFeelText.EyeTracking(lastEyeTracking);
        UpdateEyeTrackingMarker();

        SpeechToTextText.Text = UFeelText.SpeechToText(snapshot.SpeechToText);
        HeartRateText.Text = UFeelText.HeartRate(snapshot.HeartRate);
    }

    private void UpdateEyeTrackingMarker()
    {
        EyeTrackingOverlay.IsVisible = lastEyeTracking.IsValid && !CalibrationOverlay.IsVisible;
        if (EyeTrackingOverlay.IsVisible)
            PlaceMarker(EyeTrackingCanvas, EyeTrackingMarker, lastEyeTracking.EstimatedX, lastEyeTracking.EstimatedY);
    }

    private static void PlaceMarker(Canvas canvas, Control marker, float x, float y)
    {
        double width = canvas.Bounds.Width;
        double height = canvas.Bounds.Height;
        if (width <= 0 || height <= 0)
            return;

        Canvas.SetLeft(marker, x * width - marker.Width / 2);
        Canvas.SetTop(marker, y * height - marker.Height / 2);
    }
}
