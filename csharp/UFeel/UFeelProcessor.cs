using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace UFeel;

public sealed class UFeelProcessor : IDisposable
{
    private const DetectionType AllDetection =
        DetectionType.Emotions |
        DetectionType.EyeTracking |
        DetectionType.SpeechToText |
        DetectionType.HeartRate;

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

    public bool Update()
    {
        UpdateFrame();
        return Update(AllDetection);
    }

    public void UpdateFrame()
    {
        _ = UFeelNative.ufeel_update_frame(Handle);
    }

    public bool Update(DetectionType detection)
    {
        return UFeelNative.ufeel_update(Handle, (int)detection) != 0;
    }

    public FrameData? GetFrame()
    {
        IntPtr framePointer = UFeelNative.ufeel_get_frame(Handle);
        if (framePointer == IntPtr.Zero)
            return null;

        try
        {
            UFeelNative.NativeFrame frame =
                Marshal.PtrToStructure<UFeelNative.NativeFrame>(framePointer);

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
            UFeelNative.ufeel_free_frame(framePointer);
        }
    }

    public Dictionary<string, float> GetEmotions()
    {
        IntPtr result = UFeelNative.ufeel_create_emotions_result();
        if (result == IntPtr.Zero)
            return [];

        // TODO: use result fully (including status)

        try
        {
            UFeelNative.ufeel_get_emotions_result(Handle, result);
            // TODO: check status

            UFeelNative.NativeEmotionResult emotionsResult =
                Marshal.PtrToStructure<UFeelNative.NativeEmotionResult>(result);

            if (emotionsResult.Count <= 0)
            {
                return [];
            }

            var emotions = new Dictionary<string, float>(emotionsResult.Count);
            int itemSize = Marshal.SizeOf<UFeelNative.NativeEmotion>();

            for (int i = 0; i < emotionsResult.Count; i++)
            {
                IntPtr itemPointer = IntPtr.Add(
                    emotionsResult.Emotions, checked(i * itemSize));

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
            UFeelNative.ufeel_free_emotions_result(result);
        }
    }

    public EyeTrackingResultData GetEyeTracking()
    {
        IntPtr result = UFeelNative.ufeel_create_eye_tracking_result();

        if (result == IntPtr.Zero)
            return new EyeTrackingResultData(
                EyeTrackingStatus.Disabled,
                0f, 0f, 0f, 0f, 0f,
                EyeTrackingDirection.None
            );

        try
        {
            UFeelNative.Status status =
                (UFeelNative.Status)UFeelNative.ufeel_get_eye_tracking_result(Handle, result);

            if (status != UFeelNative.Status.Ok &&
                status != UFeelNative.Status.NoEyeTracking)
            {
                ThrowIfFailed(status, "retrieve the gaze estimate");
            }

            UFeelNative.NativeEyeTrackingResult eyeTrackingResult =
                Marshal.PtrToStructure<UFeelNative.NativeEyeTrackingResult>(result);

            return new EyeTrackingResultData(
                (EyeTrackingStatus)eyeTrackingResult.Status,
                eyeTrackingResult.X,
                eyeTrackingResult.Y,
                eyeTrackingResult.RecordedX,
                eyeTrackingResult.RecordedY,
                eyeTrackingResult.Confidence,
                (EyeTrackingDirection)eyeTrackingResult.Directions);
        }
        finally
        {
            UFeelNative.ufeel_free_eye_tracking_result(result);
        }
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
        return false;
    }

    public void CancelEyeTrackingCalibration()
    {
        ThrowIfFailed(
            (UFeelNative.Status)
                UFeelNative.ufeel_cancel_eye_tracking_calibration(Handle),
            "cancel eye-tracking calibration");
    }

    public EyeTrackingCalibrationProgress GetEyeTrackingCalibrationProgress()
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

    public EyeTrackingCalibrationDiagnostics GetEyeTrackingCalibrationDiagnostics()
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
        IntPtr result = UFeelNative.ufeel_create_speech_to_text_result();
        if (result == IntPtr.Zero)
            return string.Empty;

        // TODO: use result fully (including status)

        try
        {
            UFeelNative.ufeel_get_speech_to_text_result(Handle, result);

            // TODO: check status

            UFeelNative.NativeSpeechToTextResult speechToTextResult =
                Marshal.PtrToStructure<UFeelNative.NativeSpeechToTextResult>(result);

            return Marshal.PtrToStringUTF8(speechToTextResult.Speech) ?? string.Empty;
        }
        finally
        {
            UFeelNative.ufeel_free_speech_to_text_result(result); // TODO: keep the same struct for the whole program to avoid spamming alloc/free
        }
    }

    public HeartRateResultData GetHeartRate()
    {
        IntPtr result = UFeelNative.ufeel_create_heart_rate_result();
        if (result == IntPtr.Zero)
            return new HeartRateResultData(
                HeartRateStatus.RequestFailed, 0
            );

        try
        {
            UFeelNative.Status status =
                (UFeelNative.Status)UFeelNative.ufeel_get_heart_rate_result(
                    Handle, result);

            UFeelNative.NativeHeartRateResult heartRateResult =
                Marshal.PtrToStructure<UFeelNative.NativeHeartRateResult>(result);

            if (status != UFeelNative.Status.Ok &&
                status != UFeelNative.Status.NoHeartRate)
            {
                ThrowIfFailed(status, "retrieve the heartrate value");
            }

            return new HeartRateResultData(
                (HeartRateStatus)heartRateResult.Status,
                heartRateResult.Bpm);
        }
        finally
        {
            UFeelNative.ufeel_free_heart_rate_result(result);
        }
    }

    private IntPtr Handle
    {
        get
        {
            ObjectDisposedException.ThrowIf(
                disposed || handle == IntPtr.Zero, this);

            return handle;
        }
    }

    private static void ValidatePath(string path, string parameterName)
    {
        if (string.IsNullOrWhiteSpace(path))
            throw new ArgumentException(
                "A non-empty path is required.", parameterName);
    }

    private static void ThrowIfFailed(
        UFeelNative.Status status, string operation)
    {
        if (status == UFeelNative.Status.Ok)
            return;

        string message = status switch
        {
            UFeelNative.Status.NoEyeTracking =>
                "A valid gaze estimate is not available.",
            UFeelNative.Status.NoHeartRate =>
                "A valid heartrate is not available.",
            UFeelNative.Status.InvalidArgument =>
                "One or more arguments are invalid.",
            UFeelNative.Status.InvalidState =>
                "The processor is not in a state that permits this operation.",
            UFeelNative.Status.EyeTrackingCalibrationFailed =>
                "Eye-tracking calibration failed.",
            UFeelNative.Status.EyeTrackingProfileFailed =>
                "The eye-tracking calibration profile operation failed.",
            UFeelNative.Status.InternalError =>
                "An internal native-library error occurred.",
            _ => "An unknown native-library error occurred."
        };

        throw new InvalidOperationException(
            $"Failed to {operation}: {message} (status {(int)status}).");
    }

    public void Dispose()
    {
        if (disposed)
            return;

        disposed = true;

        if (handle != IntPtr.Zero)
        {
            UFeelNative.ufeel_destroy(handle);
            handle = IntPtr.Zero;
        }

        GC.SuppressFinalize(this);
    }
}
