#nullable enable

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UFeel.Interop;

namespace UFeel.Native
{
    internal sealed class MicrophonesReader : IDisposable
    {
        private static readonly int ItemSize = Marshal.SizeOf<NativeMicrophone>();

        private readonly NativeResultBuffer buffer = new NativeResultBuffer(
            NativeMethods.ufeel_create_microphones_result,
            NativeMethods.ufeel_free_microphones_result,
            "microphones");

        /// <summary>Empty when the library is built without speech-to-text.</summary>
        public IReadOnlyList<MicrophoneDevice> Read(IntPtr processor)
        {
            var status = (UFeelStatus)NativeMethods.ufeel_get_microphones_result(processor, buffer.Pointer);
            if (status == UFeelStatus.UnsupportedFeature)
                return Array.Empty<MicrophoneDevice>();

            UFeelException.ThrowIfFailed(status, "list the microphones");

            NativeMicrophonesResult result = Marshal.PtrToStructure<NativeMicrophonesResult>(buffer.Pointer);

            if (result.Count < 0 || (result.Count > 0 && result.Microphones == IntPtr.Zero))
                throw new InvalidOperationException("The native microphones result has an invalid buffer.");

            var microphones = new MicrophoneDevice[result.Count];
            for (int i = 0; i < result.Count; i++)
            {
                NativeMicrophone item = Marshal.PtrToStructure<NativeMicrophone>(
                    IntPtr.Add(result.Microphones, checked(i * ItemSize)));

                string name = Marshal.PtrToStringUTF8(item.Name) ??
                    throw new InvalidOperationException("The native microphones result has a null name.");

                microphones[i] = new MicrophoneDevice(name, item.IsDefault != 0);
            }

            return microphones;
        }

        public void Dispose() => buffer.Dispose();
    }
}
