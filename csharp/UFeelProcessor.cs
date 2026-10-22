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
        UFeelNative.Status status =
            (UFeelNative.Status)UFeelNative.ufeel_get_heartrate(
                Handle, out UFeelNative.NativeHeartRateResult result);

        if (status != UFeelNative.Status.Ok &&
            status != UFeelNative.Status.NoHeartRate)
        {
            ThrowIfFailed(status, "retrieve the heartrate value");
        }

        return new HeartRateResultData(
            (HeartRateStatus)result.Status,
            result.Bpm);
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
            UFeelNative.Status.NoGaze =>
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

    public int GetHeartRate()
    {
        IntPtr bpm =
            UFeelNative.ufeel_get_heartrate(processor);

        return (int)bpm;
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
            UFeelNative.Status.NoGaze =>
                "A valid gaze estimate is not available.",
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

    public int GetHeartRate()
    {
        int status =
            UFeelNative.ufeel_get_heartrate(Handle, out UFeelNative.NativeHeartRateResult result);

        return (int)bpm;
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
            UFeelNative.Status.NoGaze =>
                "A valid gaze estimate is not available.",
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

        if (handle != IntPtr.Zero)
        {
            UFeelNative.ufeel_destroy(handle);
            handle = IntPtr.Zero;
        }

        GC.SuppressFinalize(this);
    }
}

public sealed class FrameData
{
    public byte[] Data { get; }
    public int Width { get; }
    public int Height { get; }
    public int Channels { get; }
    public int Stride { get; }

    public FrameData(
        byte[] data, int width, int height, int channels, int stride)
    {
        Data = data;
        Width = width;
        Height = height;
        Channels = channels;
        Stride = stride;
    }
}

public sealed record GazeResultData(
    GazeStatus Status,
    float EstimatedX,
    float EstimatedY,
    float RecordedX,
    float RecordedY,
    float Confidence,
    GazeDirection Directions)
{
    public bool IsValid => Status == GazeStatus.Valid;
}

public sealed class EyeTrackingCalibrationProgress
{
    public int TargetsWithEnoughSamples { get; }
    public int TargetCount { get; }
    public int SamplesForCurrentTarget { get; }
    public int RequiredSamplesPerTarget { get; }

    public EyeTrackingCalibrationProgress(
        int targetsWithEnoughSamples,
        int targetCount,
        int samplesForCurrentTarget,
        int requiredSamplesPerTarget)
    {
        TargetsWithEnoughSamples = targetsWithEnoughSamples;
        TargetCount = targetCount;
        SamplesForCurrentTarget = samplesForCurrentTarget;
        RequiredSamplesPerTarget = requiredSamplesPerTarget;
    }
}

public sealed class FrameData
{
    public byte[] Data { get; }
    public int Width { get; }
    public int Height { get; }
    public int Channels { get; }
    public int Stride { get; }

    public FrameData(
        byte[] data, int width, int height, int channels, int stride)
    {
        Data = data;
        Width = width;
        Height = height;
        Channels = channels;
        Stride = stride;
    }
}

public sealed record GazeResultData(
    GazeStatus Status,
    float EstimatedX,
    float EstimatedY,
    float RecordedX,
    float RecordedY,
    float Confidence,
    GazeDirection Directions)
{
    public bool IsValid => Status == GazeStatus.Valid;
}

public sealed class EyeTrackingCalibrationProgress
{
    public int TargetsWithEnoughSamples { get; }
    public int TargetCount { get; }
    public int SamplesForCurrentTarget { get; }
    public int RequiredSamplesPerTarget { get; }

    public EyeTrackingCalibrationProgress(
        int targetsWithEnoughSamples,
        int targetCount,
        int samplesForCurrentTarget,
        int requiredSamplesPerTarget)
    {
        TargetsWithEnoughSamples = targetsWithEnoughSamples;
        TargetCount = targetCount;
        SamplesForCurrentTarget = samplesForCurrentTarget;
        RequiredSamplesPerTarget = requiredSamplesPerTarget;
    }
}

public enum CalibrationFailureReason
{
    None = 0,
    NotCalibrating = 1,
    TooFewValidTargets = 2,
    InsufficientSamples = 3,
    InsufficientHorizontalCoverage = 4,
    InsufficientVerticalCoverage = 5,
    InsufficientSpatialGridCoverage = 6,
    RegressionFailed = 7,
    CrossValidationMeanErrorExceeded = 8,
    CrossValidationMaxErrorExceeded = 9,
    InvalidCrossValidationResult = 10
}

public sealed class EyeTrackingCalibrationDiagnostics
{
    public CalibrationFailureReason FailureReason { get; }
    public int InputTargetCount { get; }
    public int TargetsWithEnoughRawSamples { get; }
    public int TargetsWithEnoughRetainedSamples { get; }
    public int ValidTargetCount { get; }
    public int MinimumRetainedSamples { get; }
    public int MaximumRetainedSamples { get; }
    public float HorizontalSpan { get; }
    public float VerticalSpan { get; }
    public float MeanCrossValidationError { get; }
    public float MaximumCrossValidationError { get; }
    public int CrossValidationTargetsEvaluated { get; }
    public bool MeanErrorExceeded { get; }
    public bool MaximumErrorExceeded { get; }
    public int RegressionFailures { get; }
    public int InvalidTargetCount { get; }
    public int TargetsWithInsufficientRawSamples { get; }
    public int TargetsWithInsufficientRetainedSamples { get; }

    public EyeTrackingCalibrationDiagnostics(
        CalibrationFailureReason failureReason,
        int inputTargetCount,
        int targetsWithEnoughRawSamples,
        int targetsWithEnoughRetainedSamples,
        int validTargetCount,
        int minimumRetainedSamples,
        int maximumRetainedSamples,
        float horizontalSpan,
        float verticalSpan,
        float meanCrossValidationError,
        float maximumCrossValidationError,
        int crossValidationTargetsEvaluated,
        bool meanErrorExceeded,
        bool maximumErrorExceeded,
        int regressionFailures,
        int invalidTargetCount,
        int targetsWithInsufficientRawSamples,
        int targetsWithInsufficientRetainedSamples)
    {
        FailureReason = failureReason;
        InputTargetCount = inputTargetCount;
        TargetsWithEnoughRawSamples = targetsWithEnoughRawSamples;
        TargetsWithEnoughRetainedSamples = targetsWithEnoughRetainedSamples;
        ValidTargetCount = validTargetCount;
        MinimumRetainedSamples = minimumRetainedSamples;
        MaximumRetainedSamples = maximumRetainedSamples;
        HorizontalSpan = horizontalSpan;
        VerticalSpan = verticalSpan;
        MeanCrossValidationError = meanCrossValidationError;
        MaximumCrossValidationError = maximumCrossValidationError;
        CrossValidationTargetsEvaluated = crossValidationTargetsEvaluated;
        MeanErrorExceeded = meanErrorExceeded;
        MaximumErrorExceeded = maximumErrorExceeded;
        RegressionFailures = regressionFailures;
        InvalidTargetCount = invalidTargetCount;
        TargetsWithInsufficientRawSamples = targetsWithInsufficientRawSamples;
        TargetsWithInsufficientRetainedSamples = targetsWithInsufficientRetainedSamples;
    }
}

public sealed record HeartRateResultData(
    HeartRateStatus Status,
    uint Bpm)
{
    public bool IsValid => Status == HeartRateStatus.Valid;
}
