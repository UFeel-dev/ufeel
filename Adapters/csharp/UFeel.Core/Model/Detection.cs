#nullable enable

using System;

namespace UFeel
{
    [Flags]
    public enum DetectionType
    {
        None = 0,
        Emotions = 1 << 0,
        EyeTracking = 1 << 1,
        SpeechToText = 1 << 2,
        HeartRate = 1 << 3,
    }

    public enum UFeelStatus
    {
        Ok = 0,
        NoEmotions = 1,
        NoEyeTracking = 2,
        NoSpeech = 3,
        NoHeartRate = 4,
        InvalidArgument = 5,
        InvalidState = 6,
        EyeTrackingCalibrationFailed = 7,
        EyeTrackingProfileFailed = 8,
        InternalError = 9,
        InvalidProcessor = 10,
        InvalidFrame = 11,
        UnsupportedFeature = 12,
    }

    public static class DetectionTypes
    {
        public const DetectionType All =
            DetectionType.Emotions |
            DetectionType.EyeTracking |
            DetectionType.SpeechToText |
            DetectionType.HeartRate;

        /// <summary>Features that consume camera frames.</summary>
        public const DetectionType Image = DetectionType.Emotions | DetectionType.EyeTracking;

        public static readonly DetectionType[] Each =
        {
            DetectionType.Emotions,
            DetectionType.EyeTracking,
            DetectionType.SpeechToText,
            DetectionType.HeartRate,
        };

        public static int IndexOf(DetectionType feature)
        {
            for (int i = 0; i < Each.Length; i++)
            {
                if (Each[i] == feature)
                    return i;
            }

            throw new ArgumentOutOfRangeException(nameof(feature), feature, "Expected a single feature.");
        }

        public static bool Includes(this DetectionType value, DetectionType feature) => (value & feature) != 0;
    }
}
