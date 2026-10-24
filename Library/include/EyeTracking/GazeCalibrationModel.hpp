#ifndef EYETRACKING_GAZECALIBRATIONMODEL_HPP
#define EYETRACKING_GAZECALIBRATIONMODEL_HPP

#include "opencv2/core/types.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

inline constexpr std::size_t GAZE_FEATURE_COUNT = 60;
inline constexpr std::size_t GAZE_MINIMUM_CALIBRATION_TARGETS = 9;
inline constexpr std::size_t GAZE_MINIMUM_SAMPLES_PER_TARGET = 15;
inline constexpr std::size_t GAZE_MAXIMUM_SAMPLES_PER_TARGET = 120;
inline constexpr std::size_t GAZE_MAXIMUM_CALIBRATION_TARGETS = 64;

using GazeFeatureVector = std::array<double, GAZE_FEATURE_COUNT>;

struct GazeCalibrationTarget
{
    cv::Point2f target;
    std::vector<GazeFeatureVector> samples;
};

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
    InvalidCrossValidationResult
};

struct CalibrationDiagnostics
{
    CalibrationFailureReason failureReason = CalibrationFailureReason::None;

    std::size_t inputTargetCount = 0;
    std::size_t targetsWithEnoughRawSamples = 0;
    std::size_t targetsWithEnoughRetainedSamples = 0;
    std::size_t validTargetCount = 0;

    std::size_t minimumRetainedSamples = 0;
    std::size_t maximumRetainedSamples = 0;

    double horizontalSpan = 0.0;
    double verticalSpan = 0.0;

    double meanCrossValidationError = 0.0;
    double maximumCrossValidationError = 0.0;
    std::size_t crossValidationTargetsEvaluated = 0;

    bool meanErrorExceeded = false;
    bool maximumErrorExceeded = false;

    std::size_t regressionFailures = 0;
    std::size_t invalidTargetCount = 0;
    std::size_t targetsWithInsufficientRawSamples = 0;
    std::size_t targetsWithInsufficientRetainedSamples = 0;
};

[[nodiscard]] auto calibrationFailureReasonToString(CalibrationFailureReason reason) -> const char*;

class GazeCalibrationModel
{
public:

    [[nodiscard]] static auto
    fit(const std::vector<GazeCalibrationTarget>& targets,
        CalibrationDiagnostics* diagnostics = nullptr) -> std::optional<GazeCalibrationModel>;

    [[nodiscard]] auto predict(const GazeFeatureVector& features) const
        -> std::optional<cv::Point2f>;

    [[nodiscard]] auto isValid() const -> bool;
    [[nodiscard]] auto confidence() const -> float;
    [[nodiscard]] auto save(const std::string& path) const -> bool;
    [[nodiscard]] auto load(const std::string& path) -> bool;

private:

    using Coefficients = std::array<std::array<double, GAZE_FEATURE_COUNT + 1>, 2>;

    [[nodiscard]] static auto fitRegression(const std::vector<GazeCalibrationTarget>& targets)
        -> std::optional<GazeCalibrationModel>;

    [[nodiscard]] auto predictCoordinate(const GazeFeatureVector& features, int coordinate) const
        -> double;

    GazeFeatureVector means_{};
    GazeFeatureVector scales_{};
    Coefficients coefficients_{};
    double crossValidationError_ = 0.0;
    bool valid_ = false;
};

#endif // EYETRACKING_GAZECALIBRATIONMODEL_HPP
