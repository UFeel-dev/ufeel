/*
** EPITECH PROJECT, 2026
** lib_ufeel
** File description:
** UFeelNative
*/

using System;
using System.Runtime.InteropServices;

public static class UFeelNative
{
    private const string LIB = "ufeel_wrapper";

    // CONSTRUCTOR - DESTRUCTOR

    [DllImport(LIB)]
    public static extern IntPtr ufeel_create();

    [DllImport(LIB)]
    public static extern void ufeel_destroy(IntPtr processor);

    // DATA STRUCT

    [StructLayout(LayoutKind.Sequential)]
    public struct UFeelPair
    {
        public IntPtr key;
        public float value;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct UFeelBoolPair
    {
        public IntPtr key;
        [MarshalAs(UnmanagedType.I1)]
        public bool value;
    }

    // EMOTIONS

    [DllImport(LIB)]
    public static extern IntPtr ufeel_get_emotions(IntPtr processor, out int size);

    [DllImport(LIB)]
    public static extern void ufeel_free_emotions(IntPtr ptr, int size);

    // EYE TRACKING

    [DllImport(LIB)]
    public static extern IntPtr ufeel_calibrate_directions(IntPtr processor);

    [DllImport(LIB)]
    public static extern IntPtr ufeel_get_directions(IntPtr processor, out int size);

    [DllImport(LIB)]
    public static extern void ufeel_free_directions(IntPtr ptr, int size);

    // SPEECH TO TEXT

    [DllImport(LIB)]
    public static extern IntPtr ufeel_toggle_speech(IntPtr processor, bool state);

    [DllImport(LIB)]
    public static extern IntPtr ufeel_get_speech(IntPtr processor);

    [DllImport(LIB)]
    public static extern void ufeel_free_speech(IntPtr speech);

    // DEBUG

    [DllImport(LIB)]
    public static extern IntPtr ufeel_debug_get_frame(IntPtr processor);

    [DllImport(LIB)]
    public static extern void ufeel_debug_destroy_frame(IntPtr frame);

    [DllImport(LIB)]
    public static extern void ufeel_debug_show_emotions(IntPtr frame, IntPtr ptr, int size);

    [DllImport(LIB)]
    public static extern void ufeel_debug_show_directions(IntPtr frame, IntPtr ptr, int size);

    [DllImport(LIB)]
    public static extern void ufeel_debug_show_speech(IntPtr frame, IntPtr speech);

    [DllImport(LIB)]
    public static extern void ufeel_debug_show_frame(IntPtr frame);

    [DllImport(LIB)]
    public static extern int ufeel_debug_wait_key(int delay);
}