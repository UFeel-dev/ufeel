#nullable enable

using System;

namespace UFeel
{
    public sealed class EyeTrackingSmoother
    {
        private const double TimeConstantSeconds = 0.08;
        private const double ResetGapSeconds = 0.5;

        private double smoothedX;
        private double smoothedY;
        private long? lastTimestamp;

        public void Reset() => lastTimestamp = null;

        public EyeTrackingResult Smooth(EyeTrackingResult result) => Smooth(result, Clock.Now);

        public EyeTrackingResult Smooth(EyeTrackingResult result, long timestamp)
        {
            if (!result.IsValid)
            {
                Reset();
                return result;
            }

            double dt = lastTimestamp is long last ? Clock.Elapsed(last, timestamp).TotalSeconds : 0.0;
            bool restart = lastTimestamp is null || dt <= 0.0 || dt > ResetGapSeconds;
            lastTimestamp = timestamp;

            if (restart)
            {
                smoothedX = result.EstimatedX;
                smoothedY = result.EstimatedY;
                return result;
            }

            double alpha = 1.0 - Math.Exp(-dt / TimeConstantSeconds);
            smoothedX += alpha * (result.EstimatedX - smoothedX);
            smoothedY += alpha * (result.EstimatedY - smoothedY);

            return result.WithEstimate((float)smoothedX, (float)smoothedY);
        }
    }
}
