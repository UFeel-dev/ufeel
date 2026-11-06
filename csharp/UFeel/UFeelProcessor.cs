using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace UFeel;

// ! not thread-safe, the cached native result buffers are shared between calls, so callers must serialize access
public sealed class UFeelProcessor : IDisposable
{
    private const DetectionType AllDetection =
        DetectionType.Emotions |
        DetectionType.EyeTracking |
        DetectionType.SpeechToText |
        DetectionType.HeartRate;

    private IntPtr handle;
    private bool disposed;

    private IntPtr emotionsResult;
    private IntPtr eyeTrackingResult;
    private IntPtr speechToTextResult;
    private IntPtr heartRateResult;
    private FrameData? frameBuffer;

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

        try
        {
            emotionsResult = CreateResult(
                UFeelNative.ufeel_create_emotions_result, "emotions");
            eyeTrackingResult = CreateResult(
                UFeelNative.ufeel_create_eye_tracking_result, "eye-tracking");
            speechToTextResult = CreateResult(
                UFeelNative.ufeel_create_speech_to_text_result, "speech-to-text");
            heartRateResult = CreateResult(
                UFeelNative.ufeel_create_heart_rate_result, "heart-rate");
        }
        catch
        {
            ReleaseNative();
            throw;
        }
    }

    public UFeelStatus Update() => Update(AllDetection);

    public UFeelStatus UpdateFrame()
    {
        return (UFeelStatus)UFeelNative.ufeel_update_frame(Handle);
    }

    public UFeelStatus Update(DetectionType detection)
    {
        return (UFeelStatus)UFeelNative.ufeel_update(Handle, (int)detection);
    }

    public FrameData? GetFrame()
    {
        IntPtr processor = Handle;

        if ((UFeelStatus)UFeelNative.ufeel_get_frame_info(
                processor, out UFeelNative.NativeFrame info) != UFeelStatus.Ok)
        {
            return null;
        }

        if (info.Width <= 0 ||
            info.Height <= 0 ||
            info.Channels <= 0 ||
            info.Stride <= 0)
        {
            return null;
        }

        ulong expectedSize = checked((ulong)info.Stride * (ulong)info.Height);
        if (info.DataSize < expectedSize || info.DataSize > int.MaxValue)
            return null;

        int byteCount = (int)info.DataSize;

        if (frameBuffer is null ||
            frameBuffer.Data.Length != byteCount ||
            frameBuffer.Width != info.Width ||
            frameBuffer.Height != info.Height ||
            frameBuffer.Channels != info.Channels ||
            frameBuffer.Stride != info.Stride)
        {
            frameBuffer = new FrameData(
                new byte[byteCount],
                info.Width,
                info.Height,
                info.Channels,
                info.Stride);
        }

        if ((UFeelStatus)UFeelNative.ufeel_copy_frame(
                processor, frameBuffer.Data, byteCount) != UFeelStatus.Ok)
        {
            return null;
        }

        return frameBuffer;
    }

    public bool TryGetFrameSize(out int width, out int height)
    {
        width = 0;
        height = 0;

        if ((UFeelStatus)UFeelNative.ufeel_get_frame_info(
                Handle, out UFeelNative.NativeFrame info) != UFeelStatus.Ok)
        {
            return false;
        }

        if (info.Width <= 0 || info.Height <= 0)
            return false;

        width = info.Width;
        height = info.Height;
        return true;
    }

    public bool TryCopyFrameBgra(
        IntPtr destination, int destinationRowBytes, int width, int height)
    {
        return (UFeelStatus)UFeelNative.ufeel_copy_frame_bgra(
            Handle, destination, destinationRowBytes, width, height) == UFeelStatus.Ok;
    }

    public EmotionResultData GetEmotions()
    {
        IntPtr processor = Handle;

        UFeelStatus status = (UFeelStatus)UFeelNative.ufeel_get_emotions_result(
            processor, emotionsResult);
        if (status != UFeelStatus.Ok &&
            status != UFeelStatus.NoEmotions &&
            status != UFeelStatus.InvalidFrame)
        {
            ThrowIfFailed(status, "retrieve emotion estimates");
        }

        UFeelNative.NativeEmotionResult result =
            Marshal.PtrToStructure<UFeelNative.NativeEmotionResult>(emotionsResult);

        if (!Enum.IsDefined(typeof(EmotionStatus), result.Status))
            throw new InvalidOperationException(
                $"The native emotion result has an unknown status ({result.Status}).");

        if (result.Count < 0 ||
            (result.Count > 0 && result.Emotions == IntPtr.Zero))
        {
            throw new InvalidOperationException(
                "The native emotion result contains an invalid score buffer.");
        }

        var emotions = new Dictionary<string, float>(result.Count);
        int itemSize = Marshal.SizeOf<UFeelNative.NativeEmotion>();

        for (int i = 0; i < result.Count; i++)
        {
            IntPtr itemPointer = IntPtr.Add(
                result.Emotions, checked(i * itemSize));

            UFeelNative.NativeEmotion item =
                Marshal.PtrToStructure<UFeelNative.NativeEmotion>(
                    itemPointer);

            string name = Marshal.PtrToStringUTF8(item.Name) ??
                throw new InvalidOperationException(
                    "The native emotion result contains a null score name.");

            emotions[name] = item.Value;
        }

        return new EmotionResultData((EmotionStatus)result.Status, emotions);
    }

    public EyeTrackingResultData GetEyeTracking()
    {
        IntPtr processor = Handle;

        Marshal.StructureToPtr(
            default(UFeelNative.NativeEyeTrackingResult), eyeTrackingResult, false);

        UFeelStatus status =
            (UFeelStatus)UFeelNative.ufeel_get_eye_tracking_result(
                processor, eyeTrackingResult);

        if (status != UFeelStatus.Ok &&
            status != UFeelStatus.NoEyeTracking)
        {
            ThrowIfFailed(status, "retrieve the gaze estimate");
        }

        UFeelNative.NativeEyeTrackingResult result =
            Marshal.PtrToStructure<UFeelNative.NativeEyeTrackingResult>(eyeTrackingResult);

        if (!Enum.IsDefined(typeof(EyeTrackingStatus), result.Status))
            throw new InvalidOperationException(
                $"The native eye-tracking result has an unknown status ({result.Status}).");

        return new EyeTrackingResultData(
            (EyeTrackingStatus)result.Status,
            result.X,
            result.Y,
            result.RecordedX,
            result.RecordedY,
            result.Confidence,
            (EyeTrackingDirection)result.Directions);
    }

    public void BeginEyeTrackingCalibration()
    {
        ThrowIfFailed(
            (UFeelStatus)
                UFeelNative.ufeel_begin_eye_tracking_calibration(Handle),
            "begin eye-tracking calibration");
    }

    public void SetEyeTrackingCalibrationTarget(float screenX, float screenY)
    {
        ThrowIfFailed(
            (UFeelStatus)
                UFeelNative.ufeel_set_eye_tracking_calibration_target(
                    Handle, screenX, screenY),
            "set the eye-tracking calibration target");
    }

    public bool FinishEyeTrackingCalibration()
    {
        UFeelStatus status =
            (UFeelStatus)
                UFeelNative.ufeel_finish_eye_tracking_calibration(Handle);

        if (status == UFeelStatus.Ok)
            return true;

        if (status == UFeelStatus.EyeTrackingCalibrationFailed)
            return false;

        ThrowIfFailed(status, "finish eye-tracking calibration");
        return false;
    }

    public void CancelEyeTrackingCalibration()
    {
        ThrowIfFailed(
            (UFeelStatus)
                UFeelNative.ufeel_cancel_eye_tracking_calibration(Handle),
            "cancel eye-tracking calibration");
    }

    public EyeTrackingCalibrationProgress GetEyeTrackingCalibrationProgress()
    {
        ThrowIfFailed(
            (UFeelStatus)
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
            (UFeelStatus)
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

        UFeelStatus status =
            (UFeelStatus)
                UFeelNative.ufeel_save_eye_tracking_calibration_profile(
                    Handle, path);

        if (status == UFeelStatus.Ok)
            return true;

        if (status == UFeelStatus.EyeTrackingProfileFailed)
            return false;

        ThrowIfFailed(status, "save the eye-tracking calibration profile");
        return false;
    }

    public bool LoadEyeTrackingCalibrationProfile(string path)
    {
        ValidatePath(path, nameof(path));

        UFeelStatus status =
            (UFeelStatus)
                UFeelNative.ufeel_load_eye_tracking_calibration_profile(
                    Handle, path);

        if (status == UFeelStatus.Ok)
            return true;

        if (status == UFeelStatus.EyeTrackingProfileFailed)
            return false;

        ThrowIfFailed(status, "load the eye-tracking calibration profile");
        return false;
    }

    public void SetSpeechToTextEnabled(bool enabled)
    {
        ThrowIfFailed(
            (UFeelStatus)
                UFeelNative.ufeel_set_speech_to_text_enabled(
                    Handle, enabled ? 1 : 0),
            "change speech-to-text state");
    }

    public SpeechToTextResultData GetSpeechToText()
    {
        IntPtr processor = Handle;

        UFeelStatus status = (UFeelStatus)UFeelNative.ufeel_get_speech_to_text_result(
            processor, speechToTextResult);
        if (status != UFeelStatus.Ok && status != UFeelStatus.NoSpeech)
        {
            ThrowIfFailed(status, "retrieve speech-to-text result");
        }

        UFeelNative.NativeSpeechToTextResult result =
            Marshal.PtrToStructure<UFeelNative.NativeSpeechToTextResult>(speechToTextResult);

        if (!Enum.IsDefined(typeof(SpeechToTextStatus), result.Status))
            throw new InvalidOperationException(
                $"The native speech result has an unknown status ({result.Status}).");

        if (result.Speech == IntPtr.Zero)
            throw new InvalidOperationException(
                "The native speech-to-text result did not contain a string buffer.");

        return new SpeechToTextResultData(
            (SpeechToTextStatus)result.Status,
            Marshal.PtrToStringUTF8(result.Speech) ?? string.Empty);
    }

    public HeartRateResultData GetHeartRate()
    {
        IntPtr processor = Handle;

        Marshal.StructureToPtr(
            default(UFeelNative.NativeHeartRateResult), heartRateResult, false);

        UFeelStatus status =
            (UFeelStatus)UFeelNative.ufeel_get_heart_rate_result(
                processor, heartRateResult);

        UFeelNative.NativeHeartRateResult result =
            Marshal.PtrToStructure<UFeelNative.NativeHeartRateResult>(heartRateResult);

        if (status != UFeelStatus.Ok &&
            status != UFeelStatus.NoHeartRate)
        {
            ThrowIfFailed(status, "retrieve the heartrate value");
        }

        if (!Enum.IsDefined(typeof(HeartRateStatus), result.Status))
            throw new InvalidOperationException(
                $"The native heart-rate result has an unknown status ({result.Status}).");

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

    private static IntPtr CreateResult(Func<IntPtr> create, string name)
    {
        IntPtr result = create();

        if (result == IntPtr.Zero)
            throw new InvalidOperationException(
                $"Failed to allocate the native {name} result.");

        return result;
    }

    private void ReleaseNative()
    {
        if (emotionsResult != IntPtr.Zero)
        {
            UFeelNative.ufeel_free_emotions_result(emotionsResult);
            emotionsResult = IntPtr.Zero;
        }

        if (eyeTrackingResult != IntPtr.Zero)
        {
            UFeelNative.ufeel_free_eye_tracking_result(eyeTrackingResult);
            eyeTrackingResult = IntPtr.Zero;
        }

        if (speechToTextResult != IntPtr.Zero)
        {
            UFeelNative.ufeel_free_speech_to_text_result(speechToTextResult);
            speechToTextResult = IntPtr.Zero;
        }

        if (heartRateResult != IntPtr.Zero)
        {
            UFeelNative.ufeel_free_heart_rate_result(heartRateResult);
            heartRateResult = IntPtr.Zero;
        }

        if (handle != IntPtr.Zero)
        {
            UFeelNative.ufeel_destroy(handle);
            handle = IntPtr.Zero;
        }
    }

    private static void ValidatePath(string path, string parameterName)
    {
        if (string.IsNullOrWhiteSpace(path))
            throw new ArgumentException(
                "A non-empty path is required.", parameterName);
    }

    private static void ThrowIfFailed(
        UFeelStatus status, string operation)
    {
        if (status == UFeelStatus.Ok)
            return;

        string message = status switch
        {
            UFeelStatus.NoEyeTracking =>
                "A valid gaze estimate is not available.",
            UFeelStatus.NoEmotions =>
                "No emotion scores are available.",
            UFeelStatus.NoSpeech =>
                "No speech transcription is available.",
            UFeelStatus.NoHeartRate =>
                "No heart-rate reading is available.",
            UFeelStatus.InvalidProcessor =>
                "The processor handle is invalid.",
            UFeelStatus.InvalidFrame =>
                "The frame is unavailable or invalid.",
            UFeelStatus.InvalidArgument =>
                "One or more arguments are invalid.",
            UFeelStatus.InvalidState =>
                "The processor is not in a state that permits this operation.",
            UFeelStatus.EyeTrackingCalibrationFailed =>
                "Eye-tracking calibration failed.",
            UFeelStatus.EyeTrackingProfileFailed =>
                "The eye-tracking calibration profile operation failed.",
            UFeelStatus.InternalError =>
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

        ReleaseNative();

        GC.SuppressFinalize(this);
    }
}
