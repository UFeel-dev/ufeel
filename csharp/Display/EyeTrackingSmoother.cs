using System;
using System.Diagnostics;
using UFeel;

namespace UFeelTest;

internal sealed class EyeTrackingSmoother
{
    private const double SmoothingTimeConstant = 0.08;
    private const double ResetGapSeconds = 0.5;

    private double? smoothedX;
    private double? smoothedY;
    private long? lastTimestamp;

    public void Reset()
    {
        smoothedX = null;
        smoothedY = null;
        lastTimestamp = null;
    }

    public EyeTrackingResultData Smooth(EyeTrackingResultData eyeTracking)
    {
        if (!eyeTracking.IsValid)
        {
            Reset();
            return eyeTracking;
        }

        long timestamp = Stopwatch.GetTimestamp();

        double dt = lastTimestamp is null
            ? 0.0
            : (timestamp - lastTimestamp.Value) /
                (double)Stopwatch.Frequency;

        lastTimestamp = timestamp;

        if (smoothedX is null ||
            smoothedY is null ||
            dt <= 0.0 ||
            dt > ResetGapSeconds)
        {
            smoothedX = eyeTracking.EstimatedX;
            smoothedY = eyeTracking.EstimatedY;
            return eyeTracking;
        }

        double alpha = 1.0 - Math.Exp(-dt / SmoothingTimeConstant);

        smoothedX += alpha * (eyeTracking.EstimatedX - smoothedX.Value);
        smoothedY += alpha * (eyeTracking.EstimatedY - smoothedY.Value);

        return eyeTracking with
        {
            EstimatedX = (float)smoothedX.Value,
            EstimatedY = (float)smoothedY.Value
        };
    }
}
