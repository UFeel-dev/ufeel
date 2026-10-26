using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace UFeel;

public sealed class UFeelProcessor : IDisposable
{
    private IntPtr handle;
    private bool disposed;

    public UFeelProcessor(
        string modelDirectory,
        string speechModelPath,
        string? nativeLibraryPath = null)
    {
        ValidatePath(modelDirectory, nameof(modelDirectory));
        ValidatePath(speechModelPath, nameof(speechModelPath));

        if (!string.IsNullOrWhiteSpace(nativeLibraryPath))
            UFeelNative.ConfigureLibrary(nativeLibraryPath);

        handle = UFeelNative.ufeel_create(modelDirectory, speechModelPath);

        if (handle == IntPtr.Zero)
            throw new InvalidOperationException(
                "Failed to create the native UFeel processor.");
    }

    public bool Update() =>
        UFeelNative.ufeel_update(Handle) != 0;

    public FrameData? GetFrame()
    {
        IntPtr pointer = UFeelNative.ufeel_get_frame(Handle);
        if (pointer == IntPtr.Zero)
            return null;

        try
        {
            UFeelNative.NativeFrame frame =
                Marshal.PtrToStructure<UFeelNative.NativeFrame>(pointer);

            if (frame.Data == IntPtr.Zero ||
                frame.Width <= 0 ||
                frame.Height <= 0 ||
                frame.Channels <= 0 ||
                frame.Stride <= 0)
            {
                return null;
            }

            int byteCount = checked(frame.Stride * frame.Height);
            byte[] data = new byte[byteCount];
            Marshal.Copy(frame.Data, data, 0, byteCount);

            return new FrameData(
                data,
                frame.Width,
                frame.Height,
                frame.Channels,
                frame.Stride);
        }
        finally
        {
            UFeelNative.ufeel_free_frame(pointer);
        }
    }

    public Dictionary<string, float> GetEmotions()
    {
        IntPtr pointer = UFeelNative.ufeel_get_emotions(
            Handle, out int count);

        if (pointer == IntPtr.Zero || count <= 0)
        {
            if (pointer != IntPtr.Zero)
                UFeelNative.ufeel_free_emotions(pointer, count);

            return new Dictionary<string, float>();
        }

        try
        {
            var emotions = new Dictionary<string, float>(count);
            int itemSize = Marshal.SizeOf<UFeelNative.NativeEmotion>();

            for (int i = 0; i < count; i++)
            {
                IntPtr itemPointer = IntPtr.Add(
                    pointer, checked(i * itemSize));

                UFeelNative.NativeEmotion item =
                    Marshal.PtrToStructure<UFeelNative.NativeEmotion>(
                        itemPointer);

                string name =
                    Marshal.PtrToStringUTF8(item.Name) ?? string.Empty;

                emotions[name] = item.Value;
            }

            return emotions;
        }
        finally
        {
            UFeelNative.ufeel_free_emotions(pointer, count);
        }
    }

    public GazeResultData GetGaze()
    {
        UFeelNative.Status status =
            (UFeelNative.Status)UFeelNative.ufeel_get_gaze_result(
                Handle, out UFeelNative.NativeGazeResult result);

        if (status != UFeelNative.Status.Ok &&
            status != UFeelNative.Status.NoGaze)
        {
            ThrowIfFailed(status, "retrieve the gaze estimate");
        }

        return new GazeResultData(
            (GazeStatus)result.Status,
            result.X,
            result.Y,
            result.RecordedX,
            result.RecordedY,
            result.Confidence,
            (GazeDirection)result.Directions);
    }

    public void BeginEyeTrackingCalibration()
    {
        ThrowIfFailed(
            (UFeelNative.Status)
                UFeelNative.ufeel_begin_eye_tracking_calibration(Handle),
            "begin eye-tracking calibration");
    }

    public void SetEyeTrackingCalibrationTarget(float screenX, float screenY)
    {
        ThrowIfFailed(
            (UFeelNative.Status)
                UFeelNative.ufeel_set_eye_tracking_calibration_target(
                    Handle, screenX, screenY),
            "set the eye-tracking calibration target");
    }

    public bool FinishEyeTrackingCalibration()
    {
        UFeelNative.Status status =
            (UFeelNative.Status)
                UFeelNative.ufeel_finish_eye_tracking_calibration(Handle);

        if (status == UFeelNative.Status.Ok)
            return true;

        if (status == UFeelNative.Status.EyeTrackingCalibrationFailed)
            return false;

        ThrowIfFailed(status, "finish eye-tracking calibration");
        return false; // Unreachable; satisfies definite-return analysis.
    }

    public void CancelEyeTrackingCalibration()
    {
        ThrowIfFailed(
            (UFeelNative.Status)
                UFeelNative.ufeel_cancel_eye_tracking_calibration(Handle),
            "cancel eye-tracking calibration");
    }

    public EyeTrackingCalibrationProgress
        GetEyeTrackingCalibrationProgress()
    {
        ThrowIfFailed(
            (UFeelNative.Status)
                UFeelNative.ufeel_get_eye_tracking_calibration_progress(
                    Handle,
                    out UFeelNative.NativeEyeTrackingCalibrationProgress progress),
            "retrieve eye-tracking calibration progress");

        return new EyeTrackingCalibrationProgress(
            progress.TargetsWithEnoughSamples,
            progress.TargetCount,
            progress.SamplesForCurrentTarget,
            progress.RequiredSamplesPerTarget);
    }

    public EyeTrackingCalibrationDiagnostics
        GetEyeTrackingCalibrationDiagnostics()
    {
        ThrowIfFailed(
            (UFeelNative.Status)
                UFeelNative.ufeel_get_eye_tracking_calibration_diagnostics(
                    Handle,
                    out UFeelNative.NativeEyeTrackingCalibrationDiagnostics diagnostics),
            "retrieve eye-tracking calibration diagnostics");

        return new EyeTrackingCalibrationDiagnostics(
            (CalibrationFailureReason)diagnostics.FailureReason,
            diagnostics.InputTargetCount,
            diagnostics.TargetsWithEnoughRawSamples,
            diagnostics.TargetsWithEnoughRetainedSamples,
            diagnostics.ValidTargetCount,
            diagnostics.MinimumRetainedSamples,
            diagnostics.MaximumRetainedSamples,
            diagnostics.HorizontalSpan,
            diagnostics.VerticalSpan,
            diagnostics.MeanCrossValidationError,
            diagnostics.MaximumCrossValidationError,
            diagnostics.CrossValidationTargetsEvaluated,
            diagnostics.MeanErrorExceeded != 0,
            diagnostics.MaximumErrorExceeded != 0,
            diagnostics.RegressionFailures,
            diagnostics.InvalidTargetCount,
            diagnostics.TargetsWithInsufficientRawSamples,
            diagnostics.TargetsWithInsufficientRetainedSamples);
    }

    public bool SaveEyeTrackingCalibrationProfile(string path)
    {
        ValidatePath(path, nameof(path));

        UFeelNative.Status status =
            (UFeelNative.Status)
                UFeelNative.ufeel_save_eye_tracking_calibration_profile(
                    Handle, path);

        if (status == UFeelNative.Status.Ok)
            return true;

        if (status == UFeelNative.Status.EyeTrackingProfileFailed)
            return false;

        ThrowIfFailed(status, "save the eye-tracking calibration profile");
        return false;
    }

    public bool LoadEyeTrackingCalibrationProfile(string path)
    {
        ValidatePath(path, nameof(path));

        UFeelNative.Status status =
            (UFeelNative.Status)
                UFeelNative.ufeel_load_eye_tracking_calibration_profile(
                    Handle, path);

        if (status == UFeelNative.Status.Ok)
            return true;

        if (status == UFeelNative.Status.EyeTrackingProfileFailed)
            return false;

        ThrowIfFailed(status, "load the eye-tracking calibration profile");
        return false;
    }

    public void SetSpeechToTextEnabled(bool enabled)
    {
        ThrowIfFailed(
            (UFeelNative.Status)
                UFeelNative.ufeel_set_speech_to_text_enabled(
                    Handle, enabled ? 1 : 0),
            "change speech-to-text state");
    }

    public string GetSpeechToText()
    {
        IntPtr pointer = UFeelNative.ufeel_get_speech_to_text(Handle);
        if (pointer == IntPtr.Zero)
            return string.Empty;

        try
        {
            return Marshal.PtrToStringUTF8(pointer) ?? string.Empty;
        }
        finally
        {
            UFeelNative.ufeel_free_string(pointer);
        }
    }

    public HeartRateResultData GetHeartRate()
    {
        int status =
            UFeelNative.ufeel_get_heartrate(processor, out UFeelNative.NativeHeartRateResult result);

        return result.bpm;
    }

    public void Dispose()
    {
        if (disposed)
            return;

        UFeelNative.ufeel_destroy(processor);
        processor = IntPtr.Zero;
    }
}

public sealed record FrameData(
    byte[] Data,
    int Width,
    int Height,
    int Stride);
