#nullable enable

using System;
using UFeel.Interop;

namespace UFeel.Native
{
    internal sealed class FrameReader
    {
        private FrameData? buffer;

        public static bool TryGetInfo(IntPtr processor, out NativeFrame info)
        {
            return (UFeelStatus)NativeMethods.ufeel_get_frame_info(processor, out info) == UFeelStatus.Ok &&
                info.Width > 0 &&
                info.Height > 0;
        }

        /// <summary>The returned buffer is reused by the next call.</summary>
        public FrameData? Read(IntPtr processor)
        {
            if (!TryGetInfo(processor, out NativeFrame info) || info.Channels <= 0 || info.Stride <= 0)
                return null;

            ulong expectedSize = checked((ulong)info.Stride * (ulong)info.Height);
            if (info.DataSize < expectedSize || info.DataSize > int.MaxValue)
                return null;

            int byteCount = (int)info.DataSize;
            if (buffer is null ||
                buffer.Data.Length != byteCount ||
                buffer.Width != info.Width ||
                buffer.Height != info.Height ||
                buffer.Channels != info.Channels ||
                buffer.Stride != info.Stride)
            {
                buffer = new FrameData(new byte[byteCount], info.Width, info.Height, info.Channels, info.Stride);
            }

            var status = (UFeelStatus)NativeMethods.ufeel_copy_frame(processor, buffer.Data, byteCount);
            return status == UFeelStatus.Ok ? buffer : null;
        }
    }
}
