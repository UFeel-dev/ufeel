#nullable enable

using System;
using System.Runtime.InteropServices;

namespace UFeel.Interop
{
    // ! must match the structs of ufeel.h exactly

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativeFrame
    {
        public IntPtr Data;
        public int Width;
        public int Height;
        public int Channels;
        public int Stride;
        public ulong DataSize;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativeEmotion
    {
        public IntPtr Name;
        public float Value;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativeEmotionsResult
    {
        public int Status;
        public int Count;
        public IntPtr Emotions;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativeEyeTrackingResult
    {
        public int Status;
        public float X;
        public float Y;
        public float RecordedX;
        public float RecordedY;
        public float Confidence;
        public int Directions;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativeEyeTrackingCalibrationProgress
    {
        public int TargetsWithEnoughSamples;
        public int TargetCount;
        public int SamplesForCurrentTarget;
        public int RequiredSamplesPerTarget;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativeEyeTrackingCalibrationDiagnostics
    {
        public int FailureReason;
        public int InputTargetCount;
        public int TargetsWithEnoughRawSamples;
        public int TargetsWithEnoughRetainedSamples;
        public int ValidTargetCount;
        public int MinimumRetainedSamples;
        public int MaximumRetainedSamples;
        public float HorizontalSpan;
        public float VerticalSpan;
        public float MeanCrossValidationError;
        public float MaximumCrossValidationError;
        public int CrossValidationTargetsEvaluated;
        public int MeanErrorExceeded;
        public int MaximumErrorExceeded;
        public int RegressionFailures;
        public int InvalidTargetCount;
        public int TargetsWithInsufficientRawSamples;
        public int TargetsWithInsufficientRetainedSamples;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativeSpeechToTextResult
    {
        public int Status;
        public IntPtr Speech;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativeMicrophone
    {
        public IntPtr Id;
        public IntPtr Name;
        public int IsDefault;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativeMicrophonesResult
    {
        public int Count;
        public IntPtr Microphones;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct NativeHeartRateResult
    {
        public int Status;
        public uint Bpm;
    }

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeEmotionsResultHandler(IntPtr result);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void NativeEyeTrackingResultHandler(IntPtr result);
}
