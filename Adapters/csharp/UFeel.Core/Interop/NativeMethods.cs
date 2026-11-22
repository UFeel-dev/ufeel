#nullable enable

using System;
using System.Runtime.InteropServices;

namespace UFeel.Interop
{
    // ! must match the exports of ufeel.h, in header order

    internal static class NativeMethods
    {
        internal const string Library = "ufeel_wrapper";

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr ufeel_create(
            [MarshalAs(UnmanagedType.LPUTF8Str)] string modelDirectory,
            [MarshalAs(UnmanagedType.LPUTF8Str)] string speechModelPath);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void ufeel_destroy(IntPtr processor);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ufeel_update(IntPtr processor, int detectionType);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ufeel_update_from_frame(
            IntPtr processor, int detectionType, in NativeFrame frame);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ufeel_update_frame(IntPtr processor);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ufeel_set_frame(IntPtr processor, in NativeFrame frame);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr ufeel_get_frame(IntPtr processor);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void ufeel_free_frame(IntPtr frame);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ufeel_get_frame_info(IntPtr processor, out NativeFrame info);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ufeel_copy_frame(
            IntPtr processor, [Out] byte[] destination, int destinationSize);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ufeel_copy_frame_bgra(
            IntPtr processor, IntPtr destination, int destinationRowBytes, int width, int height);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ufeel_update_emotions(IntPtr processor);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ufeel_update_emotions_from_frame(
            IntPtr processor, in NativeFrame frame);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ufeel_get_emotions_result(IntPtr processor, IntPtr result);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void ufeel_get_emotions_result_from_frame(
            IntPtr processor, ref NativeFrame frame, NativeEmotionsResultHandler handler);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr ufeel_create_emotions_result();

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void ufeel_free_emotions_result(IntPtr result);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ufeel_update_eye_tracking(IntPtr processor);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ufeel_update_eye_tracking_from_frame(
            IntPtr processor, in NativeFrame frame);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ufeel_begin_eye_tracking_calibration(IntPtr processor);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ufeel_set_eye_tracking_calibration_target(
            IntPtr processor, float screenX, float screenY);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ufeel_finish_eye_tracking_calibration(IntPtr processor);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ufeel_cancel_eye_tracking_calibration(IntPtr processor);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ufeel_set_eye_tracking_head_weight(IntPtr processor, float weight);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ufeel_get_eye_tracking_result(IntPtr processor, IntPtr result);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void ufeel_get_eye_tracking_result_from_frame(
            IntPtr processor, ref NativeFrame frame, NativeEyeTrackingResultHandler handler);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ufeel_get_eye_tracking_calibration_progress(
            IntPtr processor, out NativeEyeTrackingCalibrationProgress progress);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ufeel_get_eye_tracking_calibration_diagnostics(
            IntPtr processor, out NativeEyeTrackingCalibrationDiagnostics diagnostics);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ufeel_save_eye_tracking_calibration_profile(
            IntPtr processor, [MarshalAs(UnmanagedType.LPUTF8Str)] string utf8Path);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ufeel_load_eye_tracking_calibration_profile(
            IntPtr processor, [MarshalAs(UnmanagedType.LPUTF8Str)] string utf8Path);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr ufeel_create_eye_tracking_result();

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void ufeel_free_eye_tracking_result(IntPtr result);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ufeel_update_speech_to_text(IntPtr processor);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ufeel_set_speech_to_text_enabled(IntPtr processor, int enabled);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ufeel_get_speech_to_text_result(IntPtr processor, IntPtr result);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr ufeel_create_speech_to_text_result();

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void ufeel_free_speech_to_text_result(IntPtr result);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ufeel_get_microphones_result(IntPtr processor, IntPtr result);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ufeel_set_microphone(
            IntPtr processor, [MarshalAs(UnmanagedType.LPUTF8Str)] string? utf8Id);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ufeel_get_microphones_changed(IntPtr processor, out int changed);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr ufeel_create_microphones_result();

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void ufeel_free_microphones_result(IntPtr result);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ufeel_update_heart_rate(IntPtr processor);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern int ufeel_get_heart_rate_result(IntPtr processor, IntPtr result);

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern IntPtr ufeel_create_heart_rate_result();

        [DllImport(Library, CallingConvention = CallingConvention.Cdecl)]
        internal static extern void ufeel_free_heart_rate_result(IntPtr result);
    }
}
