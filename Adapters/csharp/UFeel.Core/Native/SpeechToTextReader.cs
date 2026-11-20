#nullable enable

using System;
using System.Runtime.InteropServices;
using UFeel.Interop;

namespace UFeel.Native
{
    internal sealed class SpeechToTextReader : IDisposable
    {
        private readonly NativeResultBuffer buffer = new NativeResultBuffer(
            NativeMethods.ufeel_create_speech_to_text_result,
            NativeMethods.ufeel_free_speech_to_text_result,
            "speech-to-text");

        public SpeechToTextResult Read(IntPtr processor)
        {
            var status = (UFeelStatus)NativeMethods.ufeel_get_speech_to_text_result(processor, buffer.Pointer);
            UFeelException.ThrowUnless(status, UFeelStatus.NoSpeech, "retrieve the speech-to-text result");

            NativeSpeechToTextResult result = Marshal.PtrToStructure<NativeSpeechToTextResult>(buffer.Pointer);

            if (!Enum.IsDefined(typeof(SpeechToTextStatus), result.Status))
                throw new InvalidOperationException($"Unknown native speech status ({result.Status}).");

            if (result.Speech == IntPtr.Zero)
                throw new InvalidOperationException("The native speech-to-text result has no string buffer.");

            return new SpeechToTextResult(
                (SpeechToTextStatus)result.Status,
                Marshal.PtrToStringUTF8(result.Speech) ?? string.Empty);
        }

        public void Dispose() => buffer.Dispose();
    }
}
