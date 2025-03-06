using System.Runtime.InteropServices;
using UnityEngine;

namespace MediaPipe.EmotionDetection
{
    partial class EmotionDetector
    {
        [StructLayout(LayoutKind.Sequential)]
        public readonly struct Detection
        {
            public readonly uint classIndex;
            public readonly float score;

            public static int Size = 6 * sizeof(int);

            public override string ToString() => $"({classIndex}({score})";
        };
    }
}
