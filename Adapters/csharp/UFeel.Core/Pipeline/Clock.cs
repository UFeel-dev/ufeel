#nullable enable

using System;
using System.Diagnostics;

namespace UFeel
{
    /// <summary>Monotonic timestamps (<see cref="Stopwatch"/> ticks).</summary>
    public static class Clock
    {
        private static readonly double TicksPerTimestamp = TimeSpan.TicksPerSecond / (double)Stopwatch.Frequency;

        public static long Now => Stopwatch.GetTimestamp();

        public static TimeSpan Elapsed(long start, long end)
        {
            return TimeSpan.FromTicks((long)((end - start) * TicksPerTimestamp));
        }

        public static TimeSpan Since(long start) => Elapsed(start, Now);
    }
}
