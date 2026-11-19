#nullable enable

using System;
using System.Runtime.InteropServices;
using UFeel.Interop;

namespace UFeel.Native
{
    internal sealed class HeartRateReader : IDisposable
    {
        private readonly NativeResultBuffer buffer = new NativeResultBuffer(
            NativeMethods.ufeel_create_heart_rate_result,
            NativeMethods.ufeel_free_heart_rate_result,
            "heart-rate");

        public HeartRateResult Read(IntPtr processor)
        {
            Marshal.StructureToPtr(default(NativeHeartRateResult), buffer.Pointer, false);

            var status = (UFeelStatus)NativeMethods.ufeel_get_heart_rate_result(processor, buffer.Pointer);
            UFeelException.ThrowUnless(status, UFeelStatus.NoHeartRate, "retrieve the heart rate");

            NativeHeartRateResult result = Marshal.PtrToStructure<NativeHeartRateResult>(buffer.Pointer);

            if (!Enum.IsDefined(typeof(HeartRateStatus), result.Status))
                throw new InvalidOperationException($"Unknown native heart-rate status ({result.Status}).");

            return new HeartRateResult((HeartRateStatus)result.Status, result.Bpm);
        }

        public void Dispose() => buffer.Dispose();
    }
}
