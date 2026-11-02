using System;

namespace UFeel;

[Flags]
public enum DetectionType
{
    None = 0,
    Emotions = 1 << 0,
    EyeTracking = 1 << 1,
    SpeechToText = 1 << 2,
    HeartRate = 1 << 3,
}

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

public enum HeartRateStatus
{
    JsonParseError = 0,
    JsonParseOutOfRange = 1,
    RequestFailed = 2,
    Valid = 3,
    NoData = 4,
}

public enum CalibrationFailureReason
{
    None = 0,
    NotCalibrating = 1,
    TooFewValidTargets = 2,
    InsufficientSamples = 3,
    InsufficientHorizontalCoverage = 4,
    InsufficientVerticalCoverage = 5,
    InsufficientSpatialGridCoverage = 6,
    RegressionFailed = 7,
    CrossValidationMeanErrorExceeded = 8,
    CrossValidationMaxErrorExceeded = 9,
    InvalidCrossValidationResult = 10
}
