using System.Runtime.InteropServices;

public sealed class UFeelProcessor : IDisposable
{
    private IntPtr processor;

    public UFeelProcessor(
        string libraryPath,
        string modelDirectory,
        string speechModelPath)
    {
        NativeLibrary.SetDllImportResolver(
            typeof(UFeelNative).Assembly,
            (libraryName, assembly, searchPath) =>
            {
                if (libraryName != "ufeel_wrapper")
                    return IntPtr.Zero;

                return NativeLibrary.Load(libraryPath);
            });

        processor = UFeelNative.ufeel_create(
            modelDirectory,
            speechModelPath);

        if (processor == IntPtr.Zero)
            throw new InvalidOperationException(
                "ufeel_create failed.");
    }

    public bool Update()
    {
        return UFeelNative.ufeel_update(processor) != 0;
    }

    public FrameData? GetFrame()
    {
        IntPtr frame =
            UFeelNative.ufeel_get_frame(processor);

        if (frame == IntPtr.Zero)
            return null;

        try
        {
            var nativeFrame =
                Marshal.PtrToStructure<UFeelNative.UFeelFrame>(
                    frame);

            int byteCount =
                checked((int)(
                    nativeFrame.stride *
                    nativeFrame.height));

            byte[] data =
                new byte[byteCount];

            Marshal.Copy(
                nativeFrame.data,
                data,
                0,
                byteCount);

            return new FrameData(
                data,
                (int)nativeFrame.width,
                (int)nativeFrame.height,
                (int)nativeFrame.stride);
        }
        finally
        {
            UFeelNative.ufeel_free_frame(frame);
        }
    }

    public Dictionary<string, float> GetEmotions()
    {
        IntPtr emotions =
            UFeelNative.ufeel_get_emotions(
                processor,
                out uint size);

        var result =
            new Dictionary<string, float>();

        if (emotions == IntPtr.Zero)
            return result;

        try
        {
            int structSize =
                Marshal.SizeOf<UFeelNative.UFeelPair>();

            for (uint i = 0; i < size; i++)
            {
                IntPtr item =
                    IntPtr.Add(
                        emotions,
                        checked(
                            (int)(i * (uint)structSize)));

                var pair =
                    Marshal.PtrToStructure<
                        UFeelNative.UFeelPair>(item);

                string? key =
                    Marshal.PtrToStringAnsi(pair.key);

                if (!string.IsNullOrEmpty(key))
                    result[key] = pair.value;
            }

            return result;
        }
        finally
        {
            UFeelNative.ufeel_free_emotions(
                emotions,
                size);
        }
    }

    public GazeData GetGaze()
    {
        int status = UFeelNative.ufeel_get_gaze_ex(
            processor,
            out UFeelNative.UFeelGazeEx gaze);

        if (status == (int)UFeelNative.UFeelStatus.NoGaze)
            return new GazeData(
                0.0F,
                0.0F,
                0.0F,
                UFeelNative.GazeDirection.None,
                (UFeelNative.GazeStatus)gaze.status);

        EnsureStatus(status, "get gaze");
        return new GazeData(
            gaze.x,
            gaze.y,
            gaze.confidence,
            (UFeelNative.GazeDirection)gaze.directions,
            (UFeelNative.GazeStatus)gaze.status);
    }

    public GazeResultData GetGazeResult()
    {
        int status = UFeelNative.ufeel_get_gaze_result(
            processor,
            out UFeelNative.UFeelGazeResult gaze);

        var result = new GazeResultData(
            gaze.recorded_x,
            gaze.recorded_y,
            gaze.estimated_x,
            gaze.estimated_y,
            gaze.confidence,
            (UFeelNative.GazeDirection)gaze.directions,
            (UFeelNative.GazeStatus)gaze.status);

        if (status == (int)UFeelNative.UFeelStatus.NoGaze)
            return result;

        EnsureStatus(status, "get gaze result");
        return result;
    }

    public void BeginCalibration()
    {
        EnsureStatus(
            UFeelNative.ufeel_begin_calibration(processor),
            "begin calibration");
    }

    public void SetCalibrationTarget(float x, float y)
    {
        EnsureStatus(
            UFeelNative.ufeel_set_calibration_target(processor, x, y),
            "set calibration target");
    }

    public bool FinishCalibration()
    {
        int status = UFeelNative.ufeel_finish_calibration(processor);
        if (status == (int)UFeelNative.UFeelStatus.CalibrationFailed)
            return false;

        EnsureStatus(status, "finish calibration");
        return true;
    }

    public void CancelCalibration()
    {
        EnsureStatus(
            UFeelNative.ufeel_cancel_calibration(processor),
            "cancel calibration");
    }

    public CalibrationProgress GetCalibrationProgress()
    {
        EnsureStatus(
            UFeelNative.ufeel_get_calibration_progress(
                processor,
                out UFeelNative.UFeelCalibrationProgress progress),
            "get calibration progress");
        return new CalibrationProgress(
            progress.targets_with_enough_samples,
            progress.target_count,
            progress.samples_for_current_target,
            progress.required_samples_per_target);
    }

    public bool SaveCalibrationProfile(string path)
    {
        int status = UFeelNative.ufeel_save_calibration_profile(processor, path);
        if (status == (int)UFeelNative.UFeelStatus.ProfileFailed)
            return false;

        EnsureStatus(status, "save calibration profile");
        return true;
    }

    public bool LoadCalibrationProfile(string path)
    {
        int status = UFeelNative.ufeel_load_calibration_profile(processor, path);
        if (status == (int)UFeelNative.UFeelStatus.ProfileFailed)
            return false;

        EnsureStatus(status, "load calibration profile");
        return true;
    }

    public void SetSpeechEnabled(bool enabled)
    {
        UFeelNative.ufeel_toggle_speech(
            processor,
            enabled ? (byte)1 : (byte)0);
    }

    public string GetSpeech()
    {
        IntPtr speech =
            UFeelNative.ufeel_get_speech(processor);

        if (speech == IntPtr.Zero)
            return string.Empty;

        try
        {
            return Marshal.PtrToStringAnsi(speech)
                ?? string.Empty;
        }
        finally
        {
            UFeelNative.ufeel_free_speech(speech);
        }
    }

    public uint GetHeartRate()
    {
        int status =
            UFeelNative.ufeel_get_heartrate(processor, out UFeelNative.NativeHeartRateResult result);

        return result.bpm;
    }

    public void Dispose()
    {
        if (processor == IntPtr.Zero)
            return;

        UFeelNative.ufeel_destroy(processor);
        processor = IntPtr.Zero;
    }

    private static void EnsureStatus(int status, string operation)
    {
        if (status == (int)UFeelNative.UFeelStatus.Ok)
            return;

        string reason = (UFeelNative.UFeelStatus)status switch
        {
            UFeelNative.UFeelStatus.InvalidArgument => "invalid argument",
            UFeelNative.UFeelStatus.InvalidState => "invalid operation state",
            UFeelNative.UFeelStatus.CalibrationFailed => "calibration failed",
            UFeelNative.UFeelStatus.ProfileFailed => "eye-tracking profile is invalid or inaccessible",
            UFeelNative.UFeelStatus.NoGaze => "gaze is unavailable",
            _ => $"unknown native status ({status})"
        };

        throw new InvalidOperationException(
            $"Unable to {operation}: {reason}.");
    }
}

public sealed record GazeData(
    float X,
    float Y,
    float Confidence,
    UFeelNative.GazeDirection Directions,
    UFeelNative.GazeStatus Status)
{
    public bool IsValid => Status == UFeelNative.GazeStatus.Valid;
}

public sealed record GazeResultData(
    float RecordedX,
    float RecordedY,
    float EstimatedX,
    float EstimatedY,
    float Confidence,
    UFeelNative.GazeDirection Directions,
    UFeelNative.GazeStatus Status)
{
    public bool IsValid => Status == UFeelNative.GazeStatus.Valid;
}

public sealed record CalibrationProgress(
    uint TargetsWithEnoughSamples,
    uint TargetCount,
    uint SamplesForCurrentTarget,
    uint RequiredSamplesPerTarget);

public sealed record FrameData(
    byte[] Data,
    int Width,
    int Height,
    int Stride);
