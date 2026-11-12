using System;
using System.Diagnostics;
using UFeel;

namespace UFeelTest;

internal sealed class LatestFrameCapture
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
        ArgumentNullException.ThrowIfNull(processor);

        int reads = lastCaptureTimestamp is long last
            ? Math.Clamp(
                (int)(Stopwatch.GetElapsedTime(last) / framePeriod),
                1,
                MaxQueuedFrames + 1)
            : 1;

        bool waited = false;
        int performed = 0;

        while (performed < reads)
        {
            long readStart = Stopwatch.GetTimestamp();
            UFeelStatus status = processor.UpdateFrame();
            performed++;

            if (status != UFeelStatus.Ok)
            {
                Reset();
                return status;
            }

            waited = Stopwatch.GetElapsedTime(readStart) >= BlockingReadThreshold;

            if (waited)
                break;
        }

        long now = Stopwatch.GetTimestamp();

        if (waited && performed == 1 && lastReadWaited && lastCaptureTimestamp is long previous)
            UpdateFramePeriod(Stopwatch.GetElapsedTime(previous, now));

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

        framePeriod += (sample - framePeriod) * FramePeriodSmoothing;
    }
}
