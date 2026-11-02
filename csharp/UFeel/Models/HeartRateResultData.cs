namespace UFeel;

public sealed record HeartRateResultData(
    HeartRateStatus Status,
    uint Bpm)
{
    public bool IsValid => Status == HeartRateStatus.Valid;
}
