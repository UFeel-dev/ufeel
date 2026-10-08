using System.Runtime.InteropServices;

public sealed class UFeelProcessor : IDisposable
{
    private IntPtr processor;

    public UFeelProcessor(string libraryPath)
    {
        NativeLibrary.SetDllImportResolver(
            typeof(UFeelNative).Assembly,
            (libraryName, assembly, searchPath) =>
            {
                if (libraryName != "ufeel_wrapper")
                    return IntPtr.Zero;

                return NativeLibrary.Load(libraryPath);
            });

        processor = UFeelNative.ufeel_create();

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

    public Dictionary<string, bool> GetDirections()
    {
        IntPtr directions =
            UFeelNative.ufeel_get_directions(
                processor,
                out uint size);

        var result =
            new Dictionary<string, bool>();

        if (directions == IntPtr.Zero)
            return result;

        try
        {
            int structSize =
                Marshal.SizeOf<UFeelNative.UFeelBoolPair>();

            for (uint i = 0; i < size; i++)
            {
                IntPtr item =
                    IntPtr.Add(
                        directions,
                        checked(
                            (int)(i * (uint)structSize)));

                var pair =
                    Marshal.PtrToStructure<
                        UFeelNative.UFeelBoolPair>(item);

                string? key =
                    Marshal.PtrToStringAnsi(pair.key);

                if (!string.IsNullOrEmpty(key))
                    result[key] = pair.value != 0;
            }

            return result;
        }
        finally
        {
            UFeelNative.ufeel_free_directions(
                directions,
                size);
        }
    }

    public void CalibrateDirections()
    {
        UFeelNative.ufeel_calibrate_directions(
            processor);
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

    public void Dispose()
    {
        if (processor == IntPtr.Zero)
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
