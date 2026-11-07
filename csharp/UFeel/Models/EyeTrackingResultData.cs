namespace UFeel;

public sealed record EyeTrackingResultData(
    EyeTrackingStatus Status,
    float EstimatedX,
    float EstimatedY,
    float RecordedX,
    float RecordedY,
    float Confidence,
    EyeTrackingDirection Directions)
{
    public bool IsValid => Status == EyeTrackingStatus.Valid;
}
