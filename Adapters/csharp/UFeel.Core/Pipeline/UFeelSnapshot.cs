#nullable enable

namespace UFeel
{
    public sealed class UFeelSnapshot
    {
        public static readonly UFeelSnapshot Initial = new UFeelSnapshot(
            sequence: 0,
            frameAvailable: false,
            message: null,
            emotions: EmotionResult.Unavailable(EmotionStatus.FrameUnavailable),
            eyeTracking: EyeTrackingResult.Unavailable(EyeTrackingStatus.FrameUnavailable),
            speechToText: SpeechToTextResult.Unavailable(SpeechToTextStatus.Disabled),
            heartRate: HeartRateResult.Unavailable(HeartRateStatus.NoData));

        public UFeelSnapshot(
            long sequence,
            bool frameAvailable,
            string? message,
            EmotionResult emotions,
            EyeTrackingResult eyeTracking,
            SpeechToTextResult speechToText,
            HeartRateResult heartRate)
        {
            Sequence = sequence;
            FrameAvailable = frameAvailable;
            Message = message;
            Emotions = emotions;
            EyeTracking = eyeTracking;
            SpeechToText = speechToText;
            HeartRate = heartRate;
        }

        public long Sequence { get; }

        public bool FrameAvailable { get; }

        /// <summary>Null when the last step succeeded.</summary>
        public string? Message { get; }

        public EmotionResult Emotions { get; }

        /// <summary>Smoothed estimate.</summary>
        public EyeTrackingResult EyeTracking { get; }

        public SpeechToTextResult SpeechToText { get; }
        public HeartRateResult HeartRate { get; }
    }
}
