#nullable enable

using System;

namespace UFeel
{
    public sealed class UFeelException : InvalidOperationException
    {
        public UFeelException(UFeelStatus status, string operation)
            : base($"Failed to {operation}: {Describe(status)} (status {(int)status}).")
        {
            Status = status;
        }

        public UFeelStatus Status { get; }

        internal static void ThrowIfFailed(UFeelStatus status, string operation)
        {
            if (status != UFeelStatus.Ok)
                throw new UFeelException(status, operation);
        }

        internal static void ThrowUnless(UFeelStatus status, UFeelStatus accepted, string operation)
        {
            if (status != UFeelStatus.Ok && status != accepted)
                throw new UFeelException(status, operation);
        }

        private static string Describe(UFeelStatus status)
        {
            return status switch
            {
                UFeelStatus.NoEmotions => "No emotion scores are available.",
                UFeelStatus.NoEyeTracking => "A valid gaze estimate is not available.",
                UFeelStatus.NoSpeech => "No speech transcription is available.",
                UFeelStatus.NoHeartRate => "No heart-rate reading is available.",
                UFeelStatus.InvalidArgument => "One or more arguments are invalid.",
                UFeelStatus.InvalidState => "The processor is not in a state that permits this operation.",
                UFeelStatus.EyeTrackingCalibrationFailed => "Eye-tracking calibration failed.",
                UFeelStatus.EyeTrackingProfileFailed => "The eye-tracking calibration profile operation failed.",
                UFeelStatus.InternalError => "An internal native-library error occurred.",
                UFeelStatus.InvalidProcessor => "The processor handle is invalid.",
                UFeelStatus.InvalidFrame => "The frame is unavailable or invalid.",
                UFeelStatus.UnsupportedFeature => "The feature is not built into this library.",
                _ => "An unknown native-library error occurred.",
            };
        }
    }
}
