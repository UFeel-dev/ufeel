#nullable enable

using System;

namespace UFeel
{
    /// <summary>Reads the frames the camera queued since the last capture and keeps the newest to reduce lags.</summary>
    public sealed class LatestFrameCapture
    {
        private const int MaxQueuedFrames = 4;
        private const double FramePeriodSmoothing = 0.1;

        private static readonly TimeSpan BlockingReadThreshold = TimeSpan.FromMilliseconds(4);
        private static readonly TimeSpan MinFramePeriod = TimeSpan.FromMilliseconds(5);
        private static readonly TimeSpan MaxFramePeriod = TimeSpan.FromMilliseconds(250);

        private TimeSpan framePeriod = TimeSpan.FromSeconds(1.0 / 30.0);
        private long? lastCaptureTimestamp;
        private bool lastReadWaited;

        public UFeelStatus Capture(UFeelProcessor processor)
        {
            if (processor is null)
                throw new ArgumentNullException(nameof(processor));

            // * after a pause the queue may be full of stale frames, we drain it until a read blocks
            int reads = lastCaptureTimestamp is long last
                ? Math.Max(1, Math.Min((int)(Clock.Since(last).Ticks / framePeriod.Ticks), MaxQueuedFrames + 1))
                : MaxQueuedFrames + 1;

            bool waited = false;
            int performed = 0;

            while (performed < reads)
            {
                long readStart = Clock.Now;
                UFeelStatus status = processor.UpdateFrame();
                performed++;

                if (status != UFeelStatus.Ok)
                {
                    Reset();
                    return status;
                }

                waited = Clock.Since(readStart) >= BlockingReadThreshold;
                if (waited)
                    break;
            }

            long now = Clock.Now;
            if (waited && performed == 1 && lastReadWaited && lastCaptureTimestamp is long previous)
                UpdateFramePeriod(Clock.Elapsed(previous, now));

            lastReadWaited = waited;
            lastCaptureTimestamp = now;
            return UFeelStatus.Ok;
        }

        public void Reset()
        {
            lastCaptureTimestamp = null;
            lastReadWaited = false;
        }

        private void UpdateFramePeriod(TimeSpan sample)
        {
            if (sample < MinFramePeriod || sample > MaxFramePeriod)
                return;

            framePeriod += TimeSpan.FromTicks((long)((sample - framePeriod).Ticks * FramePeriodSmoothing));
        }
    }
}
