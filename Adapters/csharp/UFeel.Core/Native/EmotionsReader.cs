#nullable enable

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UFeel.Interop;

namespace UFeel.Native
{
    internal sealed class EmotionsReader : IDisposable
    {
        private static readonly int ItemSize = Marshal.SizeOf<NativeEmotion>();

        private readonly NativeResultBuffer buffer = new NativeResultBuffer(
            NativeMethods.ufeel_create_emotions_result,
            NativeMethods.ufeel_free_emotions_result,
            "emotions");

        public EmotionResult Read(IntPtr processor)
        {
            var status = (UFeelStatus)NativeMethods.ufeel_get_emotions_result(processor, buffer.Pointer);
            if (status != UFeelStatus.Ok &&
                status != UFeelStatus.NoEmotions &&
                status != UFeelStatus.InvalidFrame)
            {
                throw new UFeelException(status, "retrieve emotion estimates");
            }

            NativeEmotionsResult result = Marshal.PtrToStructure<NativeEmotionsResult>(buffer.Pointer);

            if (!Enum.IsDefined(typeof(EmotionStatus), result.Status))
                throw new InvalidOperationException($"Unknown native emotion status ({result.Status}).");

            if (result.Count < 0 || (result.Count > 0 && result.Emotions == IntPtr.Zero))
                throw new InvalidOperationException("The native emotion result has an invalid score buffer.");

            var scores = new Dictionary<string, float>(result.Count);
            for (int i = 0; i < result.Count; i++)
            {
                NativeEmotion item = Marshal.PtrToStructure<NativeEmotion>(
                    IntPtr.Add(result.Emotions, checked(i * ItemSize)));

                string name = Marshal.PtrToStringUTF8(item.Name) ??
                    throw new InvalidOperationException("The native emotion result has a null label.");

                scores[name] = item.Value;
            }

            return new EmotionResult((EmotionStatus)result.Status, scores);
        }

        public void Dispose() => buffer.Dispose();
    }
}
