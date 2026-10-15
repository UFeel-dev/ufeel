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
    public struct UFeelBoolPair
    {
        public IntPtr key;
        public byte value;
    }

    [DllImport(LIB, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr ufeel_create(
        string speechModelPath);

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
    public static extern void ufeel_calibrate_directions(
        IntPtr processor);

    [DllImport(LIB, CallingConvention = CallingConvention.Cdecl)]
    public static extern IntPtr ufeel_get_directions(
        IntPtr processor,
        out uint size);

    [DllImport(LIB, CallingConvention = CallingConvention.Cdecl)]
    public static extern void ufeel_free_directions(
        IntPtr directions,
        uint size);

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
