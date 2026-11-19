#nullable enable

using System;
using System.Runtime.InteropServices;
using UFeel.Interop;

namespace UFeel.Native
{
    internal sealed class EyeTrackingReader : IDisposable
    {
        private readonly NativeResultBuffer buffer = new NativeResultBuffer(
            NativeMethods.ufeel_create_eye_tracking_result,
            NativeMethods.ufeel_free_eye_tracking_result,
            "eye-tracking");

        public EyeTrackingResult Read(IntPtr processor)
        {
            Marshal.StructureToPtr(default(NativeEyeTrackingResult), buffer.Pointer, false);

            var status = (UFeelStatus)NativeMethods.ufeel_get_eye_tracking_result(processor, buffer.Pointer);
            UFeelException.ThrowUnless(status, UFeelStatus.NoEyeTracking, "retrieve the gaze estimate");

            NativeEyeTrackingResult result = Marshal.PtrToStructure<NativeEyeTrackingResult>(buffer.Pointer);

            if (!Enum.IsDefined(typeof(EyeTrackingStatus), result.Status))
                throw new InvalidOperationException($"Unknown native eye-tracking status ({result.Status}).");

            return new EyeTrackingResult(
                (EyeTrackingStatus)result.Status,
                result.X,
                result.Y,
                result.RecordedX,
                result.RecordedY,
                result.Confidence,
                (EyeTrackingDirection)result.Directions);
        }

        public void Dispose() => buffer.Dispose();
    }
}
