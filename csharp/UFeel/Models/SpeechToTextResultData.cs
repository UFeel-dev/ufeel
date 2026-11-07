namespace UFeel;

public sealed record SpeechToTextResultData(
    SpeechToTextStatus Status,
    string Text)
{
    public bool IsValid => Status == SpeechToTextStatus.Valid;
}
