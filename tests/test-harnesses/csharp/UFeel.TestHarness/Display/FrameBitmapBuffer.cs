using System;
using System.Threading;
using Avalonia;
using Avalonia.Media.Imaging;
using Avalonia.Platform;
using Avalonia.Threading;
using UFeel;

namespace UFeelTest;

/// <summary>
/// Double-buffered camera preview. Frames arriving while the UI thread has not shown the
/// previous one are dropped, so the bitmap being displayed is never written.
/// </summary>
internal sealed class FrameBitmapBuffer(Action<Bitmap> show) : IFrameObserver, IDisposable
{
    private readonly WriteableBitmap?[] bitmaps = new WriteableBitmap?[2];
    private int next;
    private int pending;

    public void OnFrameCaptured(UFeelProcessor processor)
    {
        if (Volatile.Read(ref pending) != 0 || !processor.TryGetFrameSize(out int width, out int height))
            return;

        var size = new PixelSize(width, height);
        WriteableBitmap? bitmap = bitmaps[next];

        if (bitmap is null || bitmap.PixelSize != size)
        {
            bitmap?.Dispose();
            bitmap = new WriteableBitmap(size, new Vector(96, 96), PixelFormat.Bgra8888, AlphaFormat.Opaque);
            bitmaps[next] = bitmap;
        }

        using (ILockedFramebuffer locked = bitmap.Lock())
        {
            if (!processor.TryCopyFrameBgra(locked.Address, locked.RowBytes, width, height))
                return;
        }

        next = 1 - next;
        Volatile.Write(ref pending, 1);
        Dispatcher.UIThread.Post(() =>
        {
            show(bitmap);
            Volatile.Write(ref pending, 0);
        });
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
