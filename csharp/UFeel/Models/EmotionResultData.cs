using System.Collections.Generic;

namespace UFeel;

public sealed record EmotionResultData(
    EmotionStatus Status,
    IReadOnlyDictionary<string, float> Emotions)
{
    public bool IsValid => Status == EmotionStatus.Valid;
}
