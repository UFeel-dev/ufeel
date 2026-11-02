using System;
using Avalonia;
using Avalonia.Media.Imaging;
using Avalonia.Platform;
using UFeel;

namespace UFeelTest;

internal sealed class FrameBitmapBuffer : IDisposable
{
    private readonly WriteableBitmap?[] bitmaps = new WriteableBitmap?[2];
    private int next;

    public Bitmap? Update(UFeelProcessor processor)
    {
        ArgumentNullException.ThrowIfNull(processor);

        if (!processor.TryGetFrameSize(out int width, out int height))
            return null;

        var size = new PixelSize(width, height);
        WriteableBitmap? bitmap = bitmaps[next];

        if (bitmap is null || bitmap.PixelSize != size)
        {
            bitmap?.Dispose();
            bitmap = new WriteableBitmap(
                size,
                new Vector(96, 96),
                PixelFormat.Bgra8888,
                AlphaFormat.Opaque);
            bitmaps[next] = bitmap;
        }

        using (ILockedFramebuffer locked = bitmap.Lock())
        {
            if (!processor.TryCopyFrameBgra(locked.Address, locked.RowBytes, width, height))
                return null;
        }

        next = 1 - next;
        return bitmap;
    }

    public void Dispose()
    {
        for (int i = 0; i < bitmaps.Length; i++)
        {
            bitmaps[i]?.Dispose();
            bitmaps[i] = null;
        }
    }
}
