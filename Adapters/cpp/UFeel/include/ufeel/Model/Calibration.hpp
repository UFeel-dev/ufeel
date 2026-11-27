#ifndef UFEEL_MODEL_CALIBRATION_HPP
#define UFEEL_MODEL_CALIBRATION_HPP

#include <cstdint>

namespace ufeel
{

enum class CalibrationFailureReason : std::uint8_t
{
    None,
    NotCalibrating,
    TooFewValidTargets,
    InsufficientSamples,
    InsufficientHorizontalCoverage,
    InsufficientVerticalCoverage,
    InsufficientSpatialGridCoverage,
    RegressionFailed,
    CrossValidationMeanErrorExceeded,
    CrossValidationMaxErrorExceeded,
    InvalidCrossValidationResult,
};

struct CalibrationProgress
{
    int targetsWithEnoughSamples = 0;
    int targetCount = 0;
    int samplesForCurrentTarget = 0;
    int requiredSamplesPerTarget = 0;

    [[nodiscard]] auto currentTargetComplete() const -> bool
    {
        return samplesForCurrentTarget >= requiredSamplesPerTarget;
    }
};

struct CalibrationDiagnostics
{
    CalibrationFailureReason failureReason = CalibrationFailureReason::None;
    int inputTargetCount = 0;
    int targetsWithEnoughRawSamples = 0;
    int targetsWithEnoughRetainedSamples = 0;
    int validTargetCount = 0;
    int minimumRetainedSamples = 0;
    int maximumRetainedSamples = 0;
    float horizontalSpan = 0.0F;
    float verticalSpan = 0.0F;
    float meanCrossValidationError = 0.0F;
    float maximumCrossValidationError = 0.0F;
    int crossValidationTargetsEvaluated = 0;
    bool meanErrorExceeded = false;
    bool maximumErrorExceeded = false;
    int regressionFailures = 0;
    int invalidTargetCount = 0;
    int targetsWithInsufficientRawSamples = 0;
    int targetsWithInsufficientRetainedSamples = 0;
};

} // namespace ufeel

#endif // UFEEL_MODEL_CALIBRATION_HPP
