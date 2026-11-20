#nullable enable

using System;

namespace UFeel
{
    /// <summary>
    /// At most one image feature runs per tick (the most overdue), so emotions and eye tracking
    /// share the camera frame rate. Thread-safe.
    /// </summary>
    public sealed class FeatureScheduler
    {
        private readonly object sync = new object();
        private readonly TimeSpan[] intervals;
        private readonly TimeSpan?[] overrides = new TimeSpan?[DetectionTypes.Each.Length];
        private readonly long?[] lastRun = new long?[DetectionTypes.Each.Length];

        public FeatureScheduler(TimeSpan emotions, TimeSpan eyeTracking, TimeSpan speechToText, TimeSpan heartRate)
        {
            intervals = new[] { emotions, eyeTracking, speechToText, heartRate };
        }

        public static FeatureScheduler CreateDefault()
        {
            return new FeatureScheduler(
                emotions: TimeSpan.FromMilliseconds(250),
                eyeTracking: TimeSpan.FromMilliseconds(50),
                speechToText: TimeSpan.FromMilliseconds(100),
                heartRate: TimeSpan.FromSeconds(1));
        }

        /// <summary><see cref="TimeSpan.Zero"/> runs the feature every tick.</summary>
        public void SetOverride(DetectionType feature, TimeSpan interval)
        {
            lock (sync)
                overrides[DetectionTypes.IndexOf(feature)] = interval;
        }

        public void ClearOverride(DetectionType feature)
        {
            lock (sync)
                overrides[DetectionTypes.IndexOf(feature)] = null;
        }

        /// <summary>Marks the returned features as run.</summary>
        public DetectionType GetDue(long timestamp, DetectionType active)
        {
            DetectionType due = DetectionType.None;
            int imageIndex = -1;
            double imageOverdue = 0.0;

            lock (sync)
            {
                for (int i = 0; i < DetectionTypes.Each.Length; i++)
                {
                    DetectionType feature = DetectionTypes.Each[i];
                    if (!active.Includes(feature))
                        continue;

                    double overdue = Overdue(i, timestamp);
                    if (overdue < 1.0)
                        continue;

                    if (!DetectionTypes.Image.Includes(feature))
                    {
                        lastRun[i] = timestamp;
                        due |= feature;
                    }
                    else if (overdue > imageOverdue)
                    {
                        imageIndex = i;
                        imageOverdue = overdue;
                    }
                }

                if (imageIndex >= 0)
                {
                    lastRun[imageIndex] = timestamp;
                    due |= DetectionTypes.Each[imageIndex];
                }
            }

            return due;
        }

        private double Overdue(int index, long timestamp)
        {
            TimeSpan interval = overrides[index] ?? intervals[index];
            long? last = lastRun[index];

            if (last is null || interval <= TimeSpan.Zero)
                return double.PositiveInfinity;

            return Clock.Elapsed(last.Value, timestamp).Ticks / (double)interval.Ticks;
        }
    }
}
