#nullable enable

using System;
using System.Collections.Generic;

namespace UFeel
{
    public enum EmotionStatus
    {
        FaceUnavailable = 0,
        Valid = 1,
        FrameUnavailable = 2,
        InvalidProcessor = 3,
        InternalError = 4,
        UnsupportedFeature = 5,
    }

    public enum EmotionType
    {
        None,
        Angry,
        Contemptuous,
        Disgusted,
        Fearful,
        Happy,
        Neutral,
        Sad,
        Surprised,
    }

    public readonly struct EmotionResult
    {
        private static readonly IReadOnlyDictionary<string, float> NoScores =
            new Dictionary<string, float>();

        private readonly IReadOnlyDictionary<string, float>? scores;

        public EmotionResult(EmotionStatus status, IReadOnlyDictionary<string, float> scores)
        {
            Status = status;
            this.scores = scores;
            Dominant = FindDominant(scores);
        }

        public EmotionStatus Status { get; }

        /// <summary>Keyed by native label, lowercase (e.g. "happy").</summary>
        public IReadOnlyDictionary<string, float> Scores => scores ?? NoScores;

        /// <summary>Null without a valid face.</summary>
        public EmotionType? Dominant { get; }

        public bool IsValid => Status == EmotionStatus.Valid;

        public static EmotionResult Unavailable(EmotionStatus status) => new EmotionResult(status, NoScores);

        public static EmotionType Parse(string label)
        {
            return label.Length > 0 && char.IsLetter(label[0]) &&
                Enum.TryParse(label, true, out EmotionType type) && type != EmotionType.None
                ? type
                : EmotionType.None;
        }

        private static EmotionType? FindDominant(IReadOnlyDictionary<string, float> scores)
        {
            EmotionType? dominant = null;
            float best = float.NegativeInfinity;

            foreach (KeyValuePair<string, float> score in scores)
            {
                EmotionType type = Parse(score.Key);
                if (type == EmotionType.None || score.Value <= best)
                    continue;

                dominant = type;
                best = score.Value;
            }

            return dominant;
        }
    }
}
