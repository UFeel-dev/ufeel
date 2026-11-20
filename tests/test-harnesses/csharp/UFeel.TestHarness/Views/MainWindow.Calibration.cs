using System;
using System.Threading;
using Avalonia.Input;
using Avalonia.Interactivity;
using UFeel;

namespace UFeelTest;

public partial class MainWindow : ICalibrationPresenter
{
    private CancellationTokenSource? calibrationCancellation;
    private float calibrationTargetX = 0.5F;
    private float calibrationTargetY = 0.5F;

    private async void CalibrateButton_OnClick(object? sender, RoutedEventArgs e)
    {
        if (calibrationCancellation is not null)
        {
            calibrationCancellation.Cancel();
            return;
        }

        using var source = new CancellationTokenSource();
        calibrationCancellation = source;
        CalibrateButton.Content = "Cancel calibration";
        CalibrationDiagnosticsText.Text = "No calibration diagnostics yet.";

        try
        {
            CalibrationResult result = await new EyeTrackingCalibration(session, this, profiles).RunAsync(source.Token);
            if (closing)
                return;

            if (result.Succeeded)
                CalibrationDiagnosticsText.Text = "Calibration completed successfully.";
            else if (result.Diagnostics is not null)
                CalibrationDiagnosticsText.Text = result.Message;

            ShowIdleStatus(result.Message);
        }
        catch (Exception exception) when (!closing)
        {
            ShowIdleStatus(exception.Message);
        }
        catch (Exception) when (closing)
        {
        }
        finally
        {
            calibrationCancellation = null;
            if (!closing)
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

    void ICalibrationPresenter.Show()
    {
        CalibrationOverlay.IsVisible = true;
        UpdateEyeTrackingMarker();
    }

    void ICalibrationPresenter.Hide()
    {
        CalibrationOverlay.IsVisible = false;
        UpdateEyeTrackingMarker();
    }

    void ICalibrationPresenter.ShowTarget(float x, float y)
    {
        calibrationTargetX = x;
        calibrationTargetY = y;
        UpdateCalibrationMarker();
    }

    void ICalibrationPresenter.ShowInstruction(string text) => CalibrationInstruction.Text = text;

    void ICalibrationPresenter.ShowStatus(string text) => StatusText.Text = text;

    private void UpdateCalibrationMarker()
    {
        PlaceMarker(CalibrationCanvas, CalibrationMarker, calibrationTargetX, calibrationTargetY);
    }
}
