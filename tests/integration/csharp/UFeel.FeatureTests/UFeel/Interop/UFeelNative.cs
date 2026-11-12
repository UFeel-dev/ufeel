using System;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;

namespace UFeel;

internal static class UFeelNative
{
    private const string LibraryName = "ufeel_wrapper";
    private static readonly object ResolverLock = new object();

    private static string? configuredLibraryPath;
    private static bool resolverConfigured;

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativeFrame
    {
        public IntPtr Data;
        public int Width;
        public int Height;
        public int Channels;
        public int Stride;
        public ulong DataSize;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativeEmotion
    {
        public IntPtr Name;
        public float Value;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativeEmotionResult
    {
        public int Status;
        public int Count;
        public IntPtr Emotions;
    }

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void EmotionsResultHandler(IntPtr emotions);

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativeEyeTrackingResult
    {
        public int Status;
        public float X;
        public float Y;
        public float RecordedX;
        public float RecordedY;
        public float Confidence;
        public int Directions;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativeEyeTrackingCalibrationProgress
    {
        public int TargetsWithEnoughSamples;
        public int TargetCount;
        public int SamplesForCurrentTarget;
        public int RequiredSamplesPerTarget;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativeEyeTrackingCalibrationDiagnostics
    {
        public int FailureReason;

        public int InputTargetCount;
        public int TargetsWithEnoughRawSamples;
        public int TargetsWithEnoughRetainedSamples;
        public int ValidTargetCount;

        public int MinimumRetainedSamples;
        public int MaximumRetainedSamples;

        public float HorizontalSpan;
        public float VerticalSpan;

        public float MeanCrossValidationError;
        public float MaximumCrossValidationError;
        public int CrossValidationTargetsEvaluated;

        public int MeanErrorExceeded;
        public int MaximumErrorExceeded;

        public int RegressionFailures;
        public int InvalidTargetCount;
        public int TargetsWithInsufficientRawSamples;
        public int TargetsWithInsufficientRetainedSamples;
    }

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void EyeTrackingResultHandler(IntPtr gaze);

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativeSpeechToTextResult
    {
        public int Status;
        public IntPtr Speech;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativeHeartRateResult
    {
        public int Status;
        public uint Bpm;
    }

    internal static void ConfigureLibrary(string libraryPath)
    {
        if (string.IsNullOrWhiteSpace(libraryPath))
        {
            throw new ArgumentException(
                "The native UFeel library path must not be empty.",
                nameof(libraryPath));
        }

        string fullPath = Path.GetFullPath(libraryPath);

        lock (ResolverLock)
        {
            if (resolverConfigured)
            {
                if (!string.Equals(
                        configuredLibraryPath,
                        fullPath,
                        StringComparison.Ordinal))
                {
                    throw new InvalidOperationException(
                        "The native UFeel library resolver has already been configured " +
                        "with a different library path.");
                }

                return;
            }

            configuredLibraryPath = fullPath;
            NativeLibrary.SetDllImportResolver(
                typeof(UFeelNative).Assembly,
                ResolveLibrary);

            resolverConfigured = true;
        }
    }

    private static IntPtr ResolveLibrary(
        string libraryName,
        Assembly assembly,
        DllImportSearchPath? searchPath)
    {
        if (!string.Equals(libraryName, LibraryName, StringComparison.Ordinal))
        {
            return IntPtr.Zero;
        }

        string? path = configuredLibraryPath;
        if (!string.IsNullOrWhiteSpace(path))
        {
            return NativeLibrary.Load(path);
        }

        return IntPtr.Zero;
    }

    // * processor

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern IntPtr ufeel_create(
        [MarshalAs(UnmanagedType.LPUTF8Str)] string modelDirectory,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string speechModelPath);

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void ufeel_destroy(IntPtr processor);

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int ufeel_update(
        IntPtr processor,
        int detectionType);

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int ufeel_update_from_frame(
        IntPtr processor,
        int detectionType,
        in NativeFrame frame);

    // * frame

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int ufeel_update_frame(IntPtr processor);

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int ufeel_set_frame(
        IntPtr processor,
        IntPtr frame);

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern IntPtr ufeel_get_frame(IntPtr processor);

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void ufeel_free_frame(IntPtr frame);

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int ufeel_get_frame_info(
        IntPtr processor,
        out NativeFrame info);

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int ufeel_copy_frame(
        IntPtr processor,
        [Out] byte[] destination,
        int destinationSize);

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int ufeel_copy_frame_bgra(
        IntPtr processor,
        IntPtr destination,
        int destinationRowBytes,
        int width,
        int height);

    // * emotions

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int ufeel_update_emotions(IntPtr processor);

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int ufeel_update_emotions_from_frame(
        IntPtr processor,
        IntPtr frame);

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int ufeel_get_emotions_result(
        IntPtr processor,
        IntPtr result);

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void ufeel_get_emotions_result_from_frame(
        IntPtr processor,
        IntPtr frame,
        EmotionsResultHandler handler);

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern IntPtr ufeel_create_emotions_result();

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void ufeel_free_emotions_result(IntPtr emotions);

    // * eye tracking

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int ufeel_update_eye_tracking(IntPtr processor);

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int ufeel_update_eye_tracking_from_frame(IntPtr processor, IntPtr frame);

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int ufeel_begin_eye_tracking_calibration(IntPtr processor);

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int ufeel_set_eye_tracking_calibration_target(
        IntPtr processor,
        float screenX,
        float screenY);

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int ufeel_finish_eye_tracking_calibration(IntPtr processor);

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int ufeel_cancel_eye_tracking_calibration(IntPtr processor);

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int ufeel_get_eye_tracking_result(
        IntPtr processor,
        IntPtr result);

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void ufeel_get_eye_tracking_result_from_frame(
        IntPtr processor,
        IntPtr frame,
        EyeTrackingResultHandler handler);

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int ufeel_get_eye_tracking_calibration_progress(
        IntPtr processor,
        out NativeEyeTrackingCalibrationProgress progress);

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int ufeel_get_eye_tracking_calibration_diagnostics(
        IntPtr processor,
        out NativeEyeTrackingCalibrationDiagnostics diagnostics);

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int ufeel_save_eye_tracking_calibration_profile(
        IntPtr processor,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string utf8Path);

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int ufeel_load_eye_tracking_calibration_profile(
        IntPtr processor,
        [MarshalAs(UnmanagedType.LPUTF8Str)] string utf8Path);

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern IntPtr ufeel_create_eye_tracking_result();

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void ufeel_free_eye_tracking_result(IntPtr emotions);

    // * speech to text

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int ufeel_update_speech_to_text(IntPtr processor);

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int ufeel_set_speech_to_text_enabled(
        IntPtr processor,
        int enabled);

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int ufeel_get_speech_to_text_result(
        IntPtr processor,
        IntPtr result);

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern IntPtr ufeel_create_speech_to_text_result();

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void ufeel_free_speech_to_text_result(IntPtr speech);

    // * heart rate

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int ufeel_update_heart_rate(IntPtr processor);

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern int ufeel_get_heart_rate_result(
        IntPtr processor,
        IntPtr result);

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern IntPtr ufeel_create_heart_rate_result();

    [DefaultDllImportSearchPaths(DllImportSearchPath.SafeDirectories)]
    [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
    internal static extern void ufeel_free_heart_rate_result(IntPtr heartrate);
}
