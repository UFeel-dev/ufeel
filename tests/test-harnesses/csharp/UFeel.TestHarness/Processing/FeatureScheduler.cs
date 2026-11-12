using System;
using System.Collections.Generic;
using System.Diagnostics;
using UFeel;

namespace UFeelTest;

internal sealed class FeatureScheduler
{
    private static readonly DetectionType[] Features =
    [
        DetectionType.Emotions,
        DetectionType.EyeTracking,
        DetectionType.SpeechToText,
        DetectionType.HeartRate,
    ];

    private const DetectionType ImageFeatures =
        DetectionType.Emotions | DetectionType.EyeTracking;

    private readonly object sync = new();
    private readonly Dictionary<DetectionType, TimeSpan> intervals;
    private readonly Dictionary<DetectionType, TimeSpan> overrides = [];
    private readonly Dictionary<DetectionType, long> lastRun = [];

    public FeatureScheduler(
        TimeSpan emotions,
        TimeSpan eyeTracking,
        TimeSpan speechToText,
        TimeSpan heartRate)
    {
        intervals = new Dictionary<DetectionType, TimeSpan>
        {
            [DetectionType.Emotions] = emotions,
            [DetectionType.EyeTracking] = eyeTracking,
            [DetectionType.SpeechToText] = speechToText,
            [DetectionType.HeartRate] = heartRate,
        };
    }

    public static FeatureScheduler CreateDefault() => new(
        emotions: TimeSpan.FromMilliseconds(250),
        eyeTracking: TimeSpan.FromMilliseconds(50),
        speechToText: TimeSpan.FromMilliseconds(100),
        heartRate: TimeSpan.FromSeconds(1));

    public void SetOverride(DetectionType feature, TimeSpan interval)
    {
        lock (sync)
            overrides[feature] = interval;
    }

    public void ClearOverride(DetectionType feature)
    {
        lock (sync)
            overrides.Remove(feature);
    }

    public DetectionType GetDue(long timestamp)
    {
        DetectionType due = DetectionType.None;
        DetectionType imageFeature = DetectionType.None;
        double imageFeatureOverdue = 0.0;

        lock (sync)
        {
            foreach (DetectionType feature in Features)
            {
                double overdue = Overdue(feature, timestamp);
                if (overdue < 1.0)
                    continue;

                if ((feature & ImageFeatures) == 0)
                {
                    lastRun[feature] = timestamp;
                    due |= feature;
                }
                else if (overdue > imageFeatureOverdue)
                {
                    imageFeature = feature;
                    imageFeatureOverdue = overdue;
                }
            }

            if (imageFeature != DetectionType.None)
            {
                lastRun[imageFeature] = timestamp;
                due |= imageFeature;
            }
        }

        return due;
    }

    private double Overdue(DetectionType feature, long timestamp)
    {
        TimeSpan interval = overrides.TryGetValue(feature, out TimeSpan overridden)
            ? overridden
            : intervals[feature];

        if (!lastRun.TryGetValue(feature, out long last) || interval <= TimeSpan.Zero)
            return double.PositiveInfinity;

        return Stopwatch.GetElapsedTime(last, timestamp) / interval;
    }
}
