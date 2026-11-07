using System;
using System.Collections.Generic;
using UFeel;

namespace UFeelTest;

internal static class FeatureTestRunner
{
    private static readonly IReadOnlyDictionary<string, DetectionType> Features =
        new Dictionary<string, DetectionType>(StringComparer.OrdinalIgnoreCase)
        {
            ["emotions"] = DetectionType.Emotions,
            ["eye-tracking"] = DetectionType.EyeTracking,
            ["speech-to-text"] = DetectionType.SpeechToText,
            ["heart-rate"] = DetectionType.HeartRate,
        };

    public static int Run(string name)
    {
        UFeelPaths paths = UFeelPaths.Resolve();
        DetectionType selected = ParseFeature(name);

        using UFeelProcessor processor = new(
            paths.ModelDirectory,
            paths.SpeechModelPath,
            paths.LibraryPath);

        if (selected == DetectionType.None)
        {
            foreach (DetectionType feature in Features.Values)
                AssertSupported(processor, feature);

            Console.WriteLine("All feature checks passed.");
            return 0;
        }

        AssertSupported(processor, selected);

        foreach (DetectionType feature in Features.Values)
        {
            if (feature != selected)
                AssertUnsupported(processor, feature);
        }

        Console.WriteLine($"{name} feature check passed.");
        return 0;
    }

    private static DetectionType ParseFeature(string name)
    {
        if (string.Equals(name, "all", StringComparison.OrdinalIgnoreCase))
            return DetectionType.None;

        if (Features.TryGetValue(name, out DetectionType feature))
            return feature;

        throw new ArgumentException(
            "Expected all, emotions, eye-tracking, speech-to-text, or heart-rate.",
            nameof(name));
    }

    private static void AssertSupported(
        UFeelProcessor processor,
        DetectionType feature)
    {
        UFeelStatus status = processor.Update(feature);
        if (status == UFeelStatus.UnsupportedFeature)
        {
            throw new InvalidOperationException(
                $"Feature {feature} was unexpectedly unsupported.");
        }
    }

    private static void AssertUnsupported(
        UFeelProcessor processor,
        DetectionType feature)
    {
        UFeelStatus status = processor.Update(feature);
        if (status != UFeelStatus.UnsupportedFeature)
        {
            throw new InvalidOperationException(
                $"Feature {feature} returned {status} instead of UnsupportedFeature.");
        }
    }
}
