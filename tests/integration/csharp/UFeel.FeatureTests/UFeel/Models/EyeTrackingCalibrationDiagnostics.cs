namespace UFeel;

public sealed class EyeTrackingCalibrationDiagnostics
{
    public CalibrationFailureReason FailureReason { get; }
    public int InputTargetCount { get; }
    public int TargetsWithEnoughRawSamples { get; }
    public int TargetsWithEnoughRetainedSamples { get; }
    public int ValidTargetCount { get; }
    public int MinimumRetainedSamples { get; }
    public int MaximumRetainedSamples { get; }
    public float HorizontalSpan { get; }
    public float VerticalSpan { get; }
    public float MeanCrossValidationError { get; }
    public float MaximumCrossValidationError { get; }
    public int CrossValidationTargetsEvaluated { get; }
    public bool MeanErrorExceeded { get; }
    public bool MaximumErrorExceeded { get; }
    public int RegressionFailures { get; }
    public int InvalidTargetCount { get; }
    public int TargetsWithInsufficientRawSamples { get; }
    public int TargetsWithInsufficientRetainedSamples { get; }

    public EyeTrackingCalibrationDiagnostics(
        CalibrationFailureReason failureReason,
        int inputTargetCount,
        int targetsWithEnoughRawSamples,
        int targetsWithEnoughRetainedSamples,
        int validTargetCount,
        int minimumRetainedSamples,
        int maximumRetainedSamples,
        float horizontalSpan,
        float verticalSpan,
        float meanCrossValidationError,
        float maximumCrossValidationError,
        int crossValidationTargetsEvaluated,
        bool meanErrorExceeded,
        bool maximumErrorExceeded,
        int regressionFailures,
        int invalidTargetCount,
        int targetsWithInsufficientRawSamples,
        int targetsWithInsufficientRetainedSamples)
    {
        FailureReason = failureReason;
        InputTargetCount = inputTargetCount;
        TargetsWithEnoughRawSamples = targetsWithEnoughRawSamples;
        TargetsWithEnoughRetainedSamples = targetsWithEnoughRetainedSamples;
        ValidTargetCount = validTargetCount;
        MinimumRetainedSamples = minimumRetainedSamples;
        MaximumRetainedSamples = maximumRetainedSamples;
        HorizontalSpan = horizontalSpan;
        VerticalSpan = verticalSpan;
        MeanCrossValidationError = meanCrossValidationError;
        MaximumCrossValidationError = maximumCrossValidationError;
        CrossValidationTargetsEvaluated = crossValidationTargetsEvaluated;
        MeanErrorExceeded = meanErrorExceeded;
        MaximumErrorExceeded = maximumErrorExceeded;
        RegressionFailures = regressionFailures;
        InvalidTargetCount = invalidTargetCount;
        TargetsWithInsufficientRawSamples = targetsWithInsufficientRawSamples;
        TargetsWithInsufficientRetainedSamples = targetsWithInsufficientRetainedSamples;
    }
}
