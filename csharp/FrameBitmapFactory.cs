
using Avalonia;
using Avalonia.Media.Imaging;
using Avalonia.Platform;
using System;
using System.Runtime.InteropServices;

namespace UFeelTest;

internal static class FrameBitmapFactory
{
    public static Bitmap Create(FrameData frame)
    {
        ArgumentNullException.ThrowIfNull(frame);

        if (frame.Width <= 0 || frame.Height <= 0)
            throw new ArgumentException("Invalid frame dimensions.", nameof(frame));

        int sourceStride = frame.Stride;
        int rowBytes = checked(frame.Width * 4);
        int sourceRowBytes = checked(frame.Width * 3);

        if (sourceStride < sourceRowBytes)
            throw new ArgumentException("Invalid frame stride.", nameof(frame));

        if (frame.Data.Length < checked(sourceStride * frame.Height))
            throw new ArgumentException("Frame buffer is too small.", nameof(frame));

        var bitmap = new WriteableBitmap(
            new PixelSize(frame.Width, frame.Height),
            new Vector(96, 96),
            PixelFormat.Rgba8888,
            AlphaFormat.Opaque);

        try
        {
            byte[] rgba = new byte[checked(rowBytes * frame.Height)];

            for (int y = 0; y < frame.Height; y++)
            {
                int sourceRow = y * sourceStride;
                int destinationRow = y * rowBytes;

                for (int x = 0; x < frame.Width; x++)
                {
                    int source = sourceRow + x * 3;
                    int destination = destinationRow + x * 4;

                    rgba[destination] = frame.Data[source + 2];
                    rgba[destination + 1] = frame.Data[source + 1];
                    rgba[destination + 2] = frame.Data[source];
                    rgba[destination + 3] = 255;
                }
            }

            using ILockedFramebuffer locked = bitmap.Lock();

            for (int y = 0; y < frame.Height; y++)
            {
                Marshal.Copy(
                    rgba,
                    y * rowBytes,
                    locked.Address + y * locked.RowBytes,
                    rowBytes);
            }

            return bitmap;
        }
        catch
        {
            bitmap.Dispose();
            throw;
        }
    }
}
