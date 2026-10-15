using System.Runtime.InteropServices;

public static class UFeelNative
{
    private const string LIB = "ufeel_wrapper";

    [StructLayout(LayoutKind.Sequential)]
    public struct UFeelFrame
    {
        public IntPtr data;
        public uint width;
        public uint height;
        public uint stride;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct UFeelPair
    {
        public IntPtr key;
        public float value;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct UFeelGaze
    {
        public float x;
        public float y;
        public uint directions;
        public uint valid;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct UFeelGazeEx
    {
        public float x;
        public float y;
        public float confidence;
        public uint directions;
        public uint status;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct UFeelCalibrationProgress
    {
        public uint targets_with_enough_samples;
        public uint target_count;
        public uint samples_for_current_target;
        public uint required_samples_per_target;
    }

    public enum UFeelStatus
    {
        Ok = 0,
        NoGaze = 1,
        InvalidArgument = 2,
        InvalidState = 3,
        CalibrationFailed = 4,
        ProfileFailed = 5
    }

    public enum GazeStatus : uint
    {
        NotCalibrated = 0,
        FaceUnavailable = 1,
        Blinking = 2,
        LowQuality = 3,
        Calibrating = 4,
        Valid = 5,
        Disabled = 6,
        FrameUnavailable = 7
    }

    [Flags]
    public enum GazeDirection : uint
    {
        None = 0,
        Left = 1U << 0,
        Right = 1U << 1,
        Up = 1U << 2,
        Down = 1U << 3,
        Center = 1U << 4
    }

    [DllImport(LIB, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr ufeel_create(
        [MarshalAs(UnmanagedType.LPUTF8Str)] string modelDirectory,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string speechModelPath);

    [DllImport(LIB, CallingConvention = CallingConvention.Cdecl)]
    public static extern void ufeel_destroy(
        IntPtr processor);

    [DllImport(LIB, CallingConvention = CallingConvention.Cdecl)]
    public static extern int ufeel_update(
        IntPtr processor);

    [DllImport(LIB, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr ufeel_get_frame(
        IntPtr processor);

    [DllImport(LIB, CallingConvention = CallingConvention.Cdecl)]
    public static extern void ufeel_free_frame(
        IntPtr frame);

    [DllImport(LIB, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr ufeel_get_emotions(
        IntPtr processor,
        out uint size);

    [DllImport(LIB, CallingConvention = CallingConvention.Cdecl)]
    public static extern void ufeel_free_emotions(
        IntPtr emotions,
        uint size);

    [DllImport(LIB, CallingConvention = CallingConvention.Cdecl)]
    public static extern int ufeel_begin_calibration(
        IntPtr processor);

    [DllImport(LIB, CallingConvention = CallingConvention.Cdecl)]
    public static extern int ufeel_set_calibration_target(
        IntPtr processor,
        float x,
        float y);

    [DllImport(LIB, CallingConvention = CallingConvention.Cdecl)]
    public static extern int ufeel_finish_calibration(
        IntPtr processor);

    [DllImport(LIB, CallingConvention = CallingConvention.Cdecl)]
    public static extern int ufeel_cancel_calibration(
        IntPtr processor);

    [DllImport(LIB, CallingConvention = CallingConvention.Cdecl)]
    public static extern int ufeel_get_gaze(
        IntPtr processor,
        out UFeelGaze gaze);

    [DllImport(LIB, CallingConvention = CallingConvention.Cdecl)]
    public static extern int ufeel_get_gaze_ex(
        IntPtr processor,
        out UFeelGazeEx gaze);

    [DllImport(LIB, CallingConvention = CallingConvention.Cdecl)]
    public static extern int ufeel_get_calibration_progress(
        IntPtr processor,
        out UFeelCalibrationProgress progress);

    [DllImport(LIB, CallingConvention = CallingConvention.Cdecl)]
    public static extern int ufeel_save_calibration_profile(
        IntPtr processor,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string profilePath);

    [DllImport(LIB, CallingConvention = CallingConvention.Cdecl)]
    public static extern int ufeel_load_calibration_profile(
        IntPtr processor,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string profilePath);

    [DllImport(LIB, CallingConvention = CallingConvention.Cdecl)]
    public static extern void ufeel_toggle_speech(
        IntPtr processor,
        byte state);

    [DllImport(LIB, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr ufeel_get_speech(
        IntPtr processor);

    [DllImport(LIB, CallingConvention = CallingConvention.Cdecl)]
    public static extern void ufeel_free_speech(
        IntPtr speech);
}
