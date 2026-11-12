namespace UFeel;

public sealed class EyeTrackingCalibrationProgress
{
    public int TargetsWithEnoughSamples { get; }
    public int TargetCount { get; }
    public int SamplesForCurrentTarget { get; }
    public int RequiredSamplesPerTarget { get; }

    public EyeTrackingCalibrationProgress(
        int targetsWithEnoughSamples,
        int targetCount,
        int samplesForCurrentTarget,
        int requiredSamplesPerTarget)
    {
        TargetsWithEnoughSamples = targetsWithEnoughSamples;
        TargetCount = targetCount;
        SamplesForCurrentTarget = samplesForCurrentTarget;
        RequiredSamplesPerTarget = requiredSamplesPerTarget;
    }
}
