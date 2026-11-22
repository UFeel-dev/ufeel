#nullable enable

using System;

namespace UFeel
{
    public enum EyeTrackingStatus
    {
        NotCalibrated = 0,
        FaceUnavailable = 1,
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

    [Flags]
    public enum EyeState
    {
        None = 0,
        Tracked = 1 << 0,
        LeftClosed = 1 << 1,
        RightClosed = 1 << 2,
    }

    public static class EyeTrackingDirectionExtensions
    {
        /// <summary><see cref="Enum.HasFlag"/> boxes on Mono (Unity).</summary>
        public static bool HasDirection(this EyeTrackingDirection value, EyeTrackingDirection direction)
        {
            return (value & direction) != EyeTrackingDirection.None;
        }

        public static bool HasState(this EyeState value, EyeState state)
        {
            return (value & state) == state;
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
            EyeTrackingDirection directions,
            float gazeX = 0.0F,
            float gazeY = 0.0F,
            float leftOpenness = 0.0F,
            float rightOpenness = 0.0F,
            EyeState eyeState = EyeState.None,
            uint blinkCount = 0,
            float eyesClosedSeconds = 0.0F)
        {
            Status = status;
            EstimatedX = estimatedX;
            EstimatedY = estimatedY;
            RecordedX = recordedX;
            RecordedY = recordedY;
            Confidence = confidence;
            Directions = directions;
            GazeX = gazeX;
            GazeY = gazeY;
            LeftOpenness = leftOpenness;
            RightOpenness = rightOpenness;
            EyeState = eyeState;
            BlinkCount = blinkCount;
            EyesClosedSeconds = eyesClosedSeconds;
        }

        public EyeTrackingStatus Status { get; }

        public float EstimatedX { get; }
        public float EstimatedY { get; }
        public float RecordedX { get; }
        public float RecordedY { get; }
        public float Confidence { get; }

        public EyeTrackingDirection Directions { get; }

        public float GazeX { get; }

        public float GazeY { get; }

        public float LeftOpenness { get; }
        public float RightOpenness { get; }

        public EyeState EyeState { get; }

        public uint BlinkCount { get; }

        public float EyesClosedSeconds { get; }

        public bool IsValid => Status == EyeTrackingStatus.Valid;

        public bool IsTracked => EyeState.HasState(EyeState.Tracked);

        public bool AreEyesClosed => EyeState.HasState(EyeState.LeftClosed | EyeState.RightClosed);

        public static EyeTrackingResult Unavailable(EyeTrackingStatus status)
        {
            return new EyeTrackingResult(status, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, EyeTrackingDirection.None);
        }

        public EyeTrackingResult WithEstimate(float x, float y)
        {
            return new EyeTrackingResult(
                Status, x, y, RecordedX, RecordedY, Confidence, Directions,
                GazeX, GazeY, LeftOpenness, RightOpenness, EyeState, BlinkCount, EyesClosedSeconds);
        }
    }
}
