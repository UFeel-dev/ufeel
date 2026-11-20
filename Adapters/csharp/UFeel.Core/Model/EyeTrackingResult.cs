#nullable enable

using System;

namespace UFeel
{
    public enum EyeTrackingStatus
    {
        NotCalibrated = 0,
        FaceUnavailable = 1,
        Blinking = 2,
        LowQuality = 3,
        Calibrating = 4,
        Valid = 5,
        Disabled = 6,
        FrameUnavailable = 7,
        InvalidProcessor = 8,
        InternalError = 9,
    }

    [Flags]
    public enum EyeTrackingDirection
    {
        None = 0,
        Left = 1 << 0,
        Right = 1 << 1,
        Up = 1 << 2,
        Down = 1 << 3,
        Center = 1 << 4,
    }

    public static class EyeTrackingDirectionExtensions
    {
        /// <summary><see cref="Enum.HasFlag"/> boxes on Mono (Unity).</summary>
        public static bool HasDirection(this EyeTrackingDirection value, EyeTrackingDirection direction)
        {
            return (value & direction) != EyeTrackingDirection.None;
        }
    }

    /// <summary>Coordinates are normalized screen positions (0..1, origin top-left).</summary>
    public readonly struct EyeTrackingResult
    {
        public EyeTrackingResult(
            EyeTrackingStatus status,
            float estimatedX,
            float estimatedY,
            float recordedX,
            float recordedY,
            float confidence,
            EyeTrackingDirection directions)
        {
            Status = status;
            EstimatedX = estimatedX;
            EstimatedY = estimatedY;
            RecordedX = recordedX;
            RecordedY = recordedY;
            Confidence = confidence;
            Directions = directions;
        }

        public EyeTrackingStatus Status { get; }
        public float EstimatedX { get; }
        public float EstimatedY { get; }
        public float RecordedX { get; }
        public float RecordedY { get; }
        public float Confidence { get; }
        public EyeTrackingDirection Directions { get; }

        public bool IsValid => Status == EyeTrackingStatus.Valid;

        public static EyeTrackingResult Unavailable(EyeTrackingStatus status)
        {
            return new EyeTrackingResult(status, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, EyeTrackingDirection.None);
        }

        public EyeTrackingResult WithEstimate(float x, float y)
        {
            return new EyeTrackingResult(Status, x, y, RecordedX, RecordedY, Confidence, Directions);
        }
    }
}
