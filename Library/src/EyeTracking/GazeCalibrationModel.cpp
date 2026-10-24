#include "EyeTracking/GazeCalibrationModel.hpp"

#include "opencv2/core.hpp"
#include "opencv2/core/base.hpp"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <limits>
#include <numeric>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace
{

constexpr std::size_t MINIMUM_RETAINED_SAMPLES_PER_TARGET = 8;
constexpr std::size_t SPATIAL_GRID_BAND_COUNT = 3;
constexpr double RIDGE_PENALTY = 0.08;
constexpr double INTERCEPT_PENALTY = 1.0e-8;
constexpr double MAXIMUM_MEAN_CALIBRATION_ERROR = 0.35;
constexpr double MAXIMUM_TARGET_CALIBRATION_ERROR = 0.60;
constexpr char PROFILE_SIGNATURE[] = "UFEEL_GAZE_PROFILE";
constexpr std::uint32_t PROFILE_VERSION = 3;

template <typename Values>
[[nodiscard]] auto median(
    Values values
) -> double
{
    if (values.empty())
    {
        return 0.0;
    }

    std::ranges::sort(values);
    auto const middle = values.size() / 2;

    if (values.size() % 2 == 0)
    {
        return (values[middle - 1] + values[middle]) * 0.5;
    }

    return values[middle];
}

[[nodiscard]] auto isFinite(
    const GazeFeatureVector& features
) -> bool
{
    return std::ranges::all_of(features, [](double value) -> bool { return std::isfinite(value); });
}

[[nodiscard]] auto robustSamples(
    const GazeCalibrationTarget& target
) -> std::vector<GazeFeatureVector>
{
    std::vector<GazeFeatureVector> finiteSamples;
    finiteSamples.reserve(target.samples.size());

    for (auto const& sample : target.samples)
    {
        if (isFinite(sample))
        {
            finiteSamples.push_back(sample);
        }
    }

    std::vector<GazeFeatureVector> retained;
    if (finiteSamples.empty())
    {
        return retained;
    }

    std::array<double, GAZE_FEATURE_COUNT> centers{};
    std::array<double, GAZE_FEATURE_COUNT> thresholds{};

    for (std::size_t feature = 0; feature < GAZE_FEATURE_COUNT; ++feature)
    {
        std::vector<double> values;
        values.reserve(finiteSamples.size());

        for (auto const& sample : finiteSamples)
        {
            values.push_back(sample[feature]);
        }

        centers[feature] = median(values);

        std::vector<double> deviations;
        deviations.reserve(values.size());

        for (auto value : values)
        {
            deviations.push_back(std::abs(value - centers[feature]));
        }

        thresholds[feature] = std::max(0.02, 4.5 * 1.4826 * median(deviations));
    }

    retained.reserve(finiteSamples.size());

    for (auto const& sample : finiteSamples)
    {
        bool inlier = true;

        for (std::size_t feature = 0; feature < GAZE_FEATURE_COUNT; ++feature)
        {
            if (std::abs(sample[feature] - centers[feature]) > thresholds[feature])
            {
                inlier = false;
                break;
            }
        }

        if (inlier)
        {
            retained.push_back(sample);
        }
    }

    return retained;
}

[[nodiscard]] auto targetHasValidCoordinates(
    const GazeCalibrationTarget& target
) -> bool
{
    return std::isfinite(target.target.x) && std::isfinite(target.target.y) &&
           target.target.x >= 0.0F && target.target.x <= 1.0F && target.target.y >= 0.0F &&
           target.target.y <= 1.0F;
}

} // namespace

auto calibrationFailureReasonToString(
    CalibrationFailureReason reason
) -> const char*
{
    switch (reason)
    {
    case CalibrationFailureReason::None:
        return "No failure";
    case CalibrationFailureReason::NotCalibrating:
        return "Calibration was not active";
    case CalibrationFailureReason::TooFewValidTargets:
        return "Too few valid calibration targets";
    case CalibrationFailureReason::InsufficientSamples:
        return "Insufficient usable samples for one or more targets";
    case CalibrationFailureReason::InsufficientHorizontalCoverage:
        return "Calibration targets do not cover enough horizontal space";
    case CalibrationFailureReason::InsufficientVerticalCoverage:
        return "Calibration targets do not cover enough vertical space";
    case CalibrationFailureReason::InsufficientSpatialGridCoverage:
        return "Calibration targets do not cover all required spatial regions";
    case CalibrationFailureReason::RegressionFailed:
        return "Regression model fitting failed";
    case CalibrationFailureReason::CrossValidationMeanErrorExceeded:
        return "Mean cross-validation error exceeded the threshold";
    case CalibrationFailureReason::CrossValidationMaxErrorExceeded:
        return "Maximum cross-validation error exceeded the threshold";
    case CalibrationFailureReason::InvalidCrossValidationResult:
        return "Cross-validation produced an invalid result";
    }

    return "Unknown calibration failure";
}

auto GazeCalibrationModel::fit(
    const std::vector<GazeCalibrationTarget>& targets, CalibrationDiagnostics* diagnostics
) -> std::optional<GazeCalibrationModel>
{
    CalibrationDiagnostics localDiagnostics{};
    auto& report = diagnostics != nullptr ? *diagnostics : localDiagnostics;

    report = {};
    report.inputTargetCount = targets.size();
    report.minimumRetainedSamples = std::numeric_limits<std::size_t>::max();

    std::vector<GazeCalibrationTarget> validTargets;
    validTargets.reserve(targets.size());

    for (auto const& target : targets)
    {
        if (!targetHasValidCoordinates(target))
        {
            ++report.invalidTargetCount;
            continue;
        }

        bool const enoughRawSamples = target.samples.size() >= GAZE_MINIMUM_SAMPLES_PER_TARGET;

        if (enoughRawSamples)
        {
            ++report.targetsWithEnoughRawSamples;
        }
        else
        {
            ++report.targetsWithInsufficientRawSamples;
        }

        auto retained = robustSamples(target);

        if (retained.size() < MINIMUM_RETAINED_SAMPLES_PER_TARGET)
        {
            ++report.targetsWithInsufficientRetainedSamples;
            continue;
        }

        ++report.targetsWithEnoughRetainedSamples;
        report.minimumRetainedSamples = std::min(report.minimumRetainedSamples, retained.size());
        report.maximumRetainedSamples = std::max(report.maximumRetainedSamples, retained.size());

        if (enoughRawSamples)
        {
            validTargets.push_back(target);
        }
    }

    if (report.targetsWithEnoughRetainedSamples == 0)
    {
        report.minimumRetainedSamples = 0;
    }

    report.validTargetCount = validTargets.size();

    if (validTargets.size() < GAZE_MINIMUM_CALIBRATION_TARGETS)
    {
        report.failureReason = report.targetsWithInsufficientRawSamples > 0 ||
                                       report.targetsWithInsufficientRetainedSamples > 0
                                   ? CalibrationFailureReason::InsufficientSamples
                                   : CalibrationFailureReason::TooFewValidTargets;
        return std::nullopt;
    }

    auto const [minimumX, maximumX] =
        std::ranges::minmax_element(validTargets, {}, [](GazeCalibrationTarget const& target) -> float {
            return target.target.x;
        });

    auto const [minimumY, maximumY] =
        std::ranges::minmax_element(validTargets, {}, [](GazeCalibrationTarget const& target) -> float {
            return target.target.y;
        });

    report.horizontalSpan = static_cast<double>(maximumX->target.x - minimumX->target.x);
    report.verticalSpan = static_cast<double>(maximumY->target.y - minimumY->target.y);

    if (report.horizontalSpan < 0.3)
    {
        report.failureReason = CalibrationFailureReason::InsufficientHorizontalCoverage;
        return std::nullopt;
    }

    if (report.verticalSpan < 0.3)
    {
        report.failureReason = CalibrationFailureReason::InsufficientVerticalCoverage;
        return std::nullopt;
    }

    std::array<std::size_t, SPATIAL_GRID_BAND_COUNT> horizontalBands{};
    std::array<std::size_t, SPATIAL_GRID_BAND_COUNT> verticalBands{};

    for (auto const& target : validTargets)
    {
        auto const horizontalBand = std::min(
            static_cast<std::size_t>(target.target.x * SPATIAL_GRID_BAND_COUNT),
            SPATIAL_GRID_BAND_COUNT - 1
        );

        auto const verticalBand = std::min(
            static_cast<std::size_t>(target.target.y * SPATIAL_GRID_BAND_COUNT),
            SPATIAL_GRID_BAND_COUNT - 1
        );

        ++horizontalBands[horizontalBand];
        ++verticalBands[verticalBand];
    }

    if (std::ranges::any_of(horizontalBands, [](std::size_t count) -> bool { return count < 2; }) ||
        std::ranges::any_of(verticalBands, [](std::size_t count) -> bool { return count < 2; }))
    {
        report.failureReason = CalibrationFailureReason::InsufficientSpatialGridCoverage;
        return std::nullopt;
    }

    std::vector<double> targetErrors;
    targetErrors.reserve(validTargets.size());

    for (std::size_t heldOut = 0; heldOut < validTargets.size(); ++heldOut)
    {
        std::vector<GazeCalibrationTarget> trainingTargets;
        trainingTargets.reserve(validTargets.size() - 1);

        for (std::size_t index = 0; index < validTargets.size(); ++index)
        {
            if (index != heldOut)
            {
                trainingTargets.push_back(validTargets[index]);
            }
        }

        auto trainingModel = fitRegression(trainingTargets);

        if (!trainingModel)
        {
            ++report.regressionFailures;
            report.failureReason = CalibrationFailureReason::RegressionFailed;
            return std::nullopt;
        }

        auto const heldOutSamples = robustSamples(validTargets[heldOut]);

        if (heldOutSamples.size() < MINIMUM_RETAINED_SAMPLES_PER_TARGET)
        {
            report.failureReason = CalibrationFailureReason::InsufficientSamples;
            return std::nullopt;
        }

        GazeFeatureVector medianFeatures{};

        for (std::size_t feature = 0; feature < GAZE_FEATURE_COUNT; ++feature)
        {
            std::vector<double> values;
            values.reserve(heldOutSamples.size());

            for (auto const& sample : heldOutSamples)
            {
                values.push_back(sample[feature]);
            }

            medianFeatures[feature] = median(values);
        }

        auto const prediction = trainingModel->predict(medianFeatures);

        if (!prediction)
        {
            report.failureReason = CalibrationFailureReason::RegressionFailed;
            return std::nullopt;
        }

        double const error = std::hypot(
            static_cast<double>(prediction->x) - validTargets[heldOut].target.x,
            static_cast<double>(prediction->y) - validTargets[heldOut].target.y
        );

        if (!std::isfinite(error))
        {
            report.failureReason = CalibrationFailureReason::InvalidCrossValidationResult;
            return std::nullopt;
        }

        targetErrors.push_back(error);
        report.crossValidationTargetsEvaluated = targetErrors.size();
    }

    if (targetErrors.empty())
    {
        report.failureReason = CalibrationFailureReason::InvalidCrossValidationResult;
        return std::nullopt;
    }

    report.meanCrossValidationError =
        std::accumulate(targetErrors.begin(), targetErrors.end(), 0.0) /
        static_cast<double>(targetErrors.size());

    report.maximumCrossValidationError = *std::ranges::max_element(targetErrors);

    if (!std::isfinite(report.meanCrossValidationError) ||
        !std::isfinite(report.maximumCrossValidationError))
    {
        report.failureReason = CalibrationFailureReason::InvalidCrossValidationResult;
        return std::nullopt;
    }

    report.meanErrorExceeded = report.meanCrossValidationError > MAXIMUM_MEAN_CALIBRATION_ERROR;

    report.maximumErrorExceeded =
        report.maximumCrossValidationError > MAXIMUM_TARGET_CALIBRATION_ERROR;

    if (report.meanErrorExceeded || report.maximumErrorExceeded)
    {
        report.failureReason = report.meanErrorExceeded
                                   ? CalibrationFailureReason::CrossValidationMeanErrorExceeded
                                   : CalibrationFailureReason::CrossValidationMaxErrorExceeded;
        return std::nullopt;
    }

    auto model = fitRegression(validTargets);

    if (!model)
    {
        ++report.regressionFailures;
        report.failureReason = CalibrationFailureReason::RegressionFailed;
        return std::nullopt;
    }

    model->crossValidationError_ = report.meanCrossValidationError;
    report.failureReason = CalibrationFailureReason::None;

    return model;
}

auto GazeCalibrationModel::fitRegression(
    const std::vector<GazeCalibrationTarget>& targets
) -> std::optional<GazeCalibrationModel>
{
    if (targets.empty())
    {
        return std::nullopt;
    }

    std::vector<std::vector<GazeFeatureVector>> samplesByTarget;
    samplesByTarget.reserve(targets.size());

    std::size_t rowCount = 0;

    for (auto const& target : targets)
    {
        if (!targetHasValidCoordinates(target))
        {
            return std::nullopt;
        }

        auto samples = robustSamples(target);

        if (samples.size() < MINIMUM_RETAINED_SAMPLES_PER_TARGET)
        {
            return std::nullopt;
        }

        if (samples.size() > std::numeric_limits<std::size_t>::max() - rowCount)
        {
            return std::nullopt;
        }

        rowCount += samples.size();
        samplesByTarget.push_back(std::move(samples));
    }

    constexpr int columnCount = static_cast<int>(GAZE_FEATURE_COUNT + 1);

    if (rowCount > static_cast<std::size_t>(std::numeric_limits<int>::max() - columnCount))
    {
        return std::nullopt;
    }

    GazeCalibrationModel model;

    for (auto const& samples : samplesByTarget)
    {
        double const sampleWeight = 1.0 / (static_cast<double>(samples.size()) *
                                           static_cast<double>(samplesByTarget.size()));

        for (auto const& sample : samples)
        {
            for (std::size_t feature = 0; feature < GAZE_FEATURE_COUNT; ++feature)
            {
                model.means_[feature] += sample[feature] * sampleWeight;
            }
        }
    }

    for (std::size_t feature = 0; feature < GAZE_FEATURE_COUNT; ++feature)
    {
        double variance = 0.0;

        for (auto const& samples : samplesByTarget)
        {
            double const sampleWeight = 1.0 / (static_cast<double>(samples.size()) *
                                               static_cast<double>(samplesByTarget.size()));

            for (auto const& sample : samples)
            {
                double const difference = sample[feature] - model.means_[feature];
                variance += difference * difference * sampleWeight;
            }
        }

        double const standardDeviation = std::sqrt(variance);

        model.scales_[feature] = std::isfinite(standardDeviation) && standardDeviation > 1.0e-5
                                     ? standardDeviation
                                     : 1.0;
    }

    auto const augmentedRowCount = rowCount + static_cast<std::size_t>(columnCount);

    cv::Mat design(static_cast<int>(augmentedRowCount), columnCount, CV_64F, cv::Scalar(0.0));

    cv::Mat targetValues(static_cast<int>(augmentedRowCount), 2, CV_64F, cv::Scalar(0.0));

    std::size_t row = 0;

    for (std::size_t targetIndex = 0; targetIndex < samplesByTarget.size(); ++targetIndex)
    {
        auto const& samples = samplesByTarget[targetIndex];
        double const balanceWeight = 1.0 / std::sqrt(static_cast<double>(samples.size()));

        for (auto const& sample : samples)
        {
            design.at<double>(static_cast<int>(row), 0) = balanceWeight;

            for (std::size_t feature = 0; feature < GAZE_FEATURE_COUNT; ++feature)
            {
                double const standardized =
                    (sample[feature] - model.means_[feature]) / model.scales_[feature];

                design.at<double>(static_cast<int>(row), static_cast<int>(feature + 1)) =
                    balanceWeight * standardized;
            }

            targetValues.at<double>(static_cast<int>(row), 0) =
                static_cast<double>(targets[targetIndex].target.x) * balanceWeight;

            targetValues.at<double>(static_cast<int>(row), 1) =
                static_cast<double>(targets[targetIndex].target.y) * balanceWeight;

            ++row;
        }
    }

    for (int column = 0; column < columnCount; ++column)
    {
        double const penalty = column == 0 ? INTERCEPT_PENALTY : RIDGE_PENALTY;
        design.at<double>(static_cast<int>(row) + column, column) = std::sqrt(penalty);
    }

    cv::Mat solution;

    if (!cv::solve(design, targetValues, solution, cv::DECOMP_SVD))
    {
        return std::nullopt;
    }

    if (solution.rows != columnCount || solution.cols != 2)
    {
        return std::nullopt;
    }

    for (int feature = 0; feature < columnCount; ++feature)
    {
        for (int coordinate = 0; coordinate < 2; ++coordinate)
        {
            double const coefficient = solution.at<double>(feature, coordinate);

            if (!std::isfinite(coefficient))
            {
                return std::nullopt;
            }

            model.coefficients_[static_cast<std::size_t>(coordinate)]
                               [static_cast<std::size_t>(feature)] = coefficient;
        }
    }

    model.valid_ = true;
    return model;
}

auto GazeCalibrationModel::predict(
    const GazeFeatureVector& features
) const -> std::optional<cv::Point2f>
{
    if (!valid_ || !isFinite(features))
    {
        return std::nullopt;
    }

    double const x = predictCoordinate(features, 0);
    double const y = predictCoordinate(features, 1);

    if (!std::isfinite(x) || !std::isfinite(y))
    {
        return std::nullopt;
    }

    auto const floatX = static_cast<float>(x);
    auto const floatY = static_cast<float>(y);

    if (!std::isfinite(floatX) || !std::isfinite(floatY))
    {
        return std::nullopt;
    }

    return cv::Point2f(floatX, floatY);
}

auto GazeCalibrationModel::predictCoordinate(
    const GazeFeatureVector& features, int coordinate
) const -> double
{
    auto const& coefficients = coefficients_[static_cast<std::size_t>(coordinate)];
    double result = coefficients[0];

    for (std::size_t feature = 0; feature < GAZE_FEATURE_COUNT; ++feature)
    {
        result +=
            ((features[feature] - means_[feature]) / scales_[feature]) * coefficients[feature + 1];
    }

    return result;
}

auto GazeCalibrationModel::isValid() const -> bool
{
    return valid_;
}

auto GazeCalibrationModel::confidence() const -> float
{
    if (!valid_ || !std::isfinite(crossValidationError_) || crossValidationError_ < 0.0)
    {
        return 0.0F;
    }

    return static_cast<float>(std::exp(-2.5 * crossValidationError_));
}

auto GazeCalibrationModel::save(
    const std::string& path
) const -> bool
{
    if (!valid_ || path.empty() || !std::isfinite(crossValidationError_) ||
        crossValidationError_ < 0.0 || crossValidationError_ > MAXIMUM_MEAN_CALIBRATION_ERROR)
    {
        return false;
    }

    for (auto value : means_)
    {
        if (!std::isfinite(value))
        {
            return false;
        }
    }

    for (auto value : scales_)
    {
        if (!std::isfinite(value) || value <= 0.0)
        {
            return false;
        }
    }

    for (auto const& coordinate : coefficients_)
    {
        for (auto value : coordinate)
        {
            if (!std::isfinite(value))
            {
                return false;
            }
        }
    }

    std::filesystem::path const profilePath(path);
    auto temporaryPath = profilePath;
    temporaryPath += ".tmp";
    auto backupPath = profilePath;
    backupPath += ".bak";

    std::error_code error;
    std::filesystem::remove(temporaryPath, error);

    if (error)
    {
        return false;
    }

    {
        std::ofstream output(temporaryPath, std::ios::trunc);

        if (!output)
        {
            return false;
        }

        output.precision(17);
        output << PROFILE_SIGNATURE << ' ' << PROFILE_VERSION << ' ' << GAZE_FEATURE_COUNT << ' '
               << crossValidationError_ << '\n';

        for (auto value : means_)
        {
            output << value << ' ';
        }
        output << '\n';

        for (auto value : scales_)
        {
            output << value << ' ';
        }
        output << '\n';

        for (auto const& coordinate : coefficients_)
        {
            for (auto value : coordinate)
            {
                output << value << ' ';
            }
            output << '\n';
        }

        output.flush();

        if (!output)
        {
            output.close();
            std::error_code ignored;
            std::filesystem::remove(temporaryPath, ignored);
            return false;
        }
    }

    bool const hasExistingProfile = std::filesystem::exists(profilePath, error);

    if (error)
    {
        std::error_code ignored;
        std::filesystem::remove(temporaryPath, ignored);
        return false;
    }

    bool movedExistingProfile = false;

    if (hasExistingProfile)
    {
        std::filesystem::remove(backupPath, error);

        if (error)
        {
            std::error_code ignored;
            std::filesystem::remove(temporaryPath, ignored);
            return false;
        }

        std::filesystem::rename(profilePath, backupPath, error);

        if (error)
        {
            std::error_code ignored;
            std::filesystem::remove(temporaryPath, ignored);
            return false;
        }

        movedExistingProfile = true;
    }

    std::filesystem::rename(temporaryPath, profilePath, error);

    if (error)
    {
        if (movedExistingProfile)
        {
            std::error_code restoreError;
            std::filesystem::rename(backupPath, profilePath, restoreError);
        }

        std::error_code ignored;
        std::filesystem::remove(temporaryPath, ignored);
        return false;
    }

    std::filesystem::remove(backupPath, error);

    return true;
}

auto GazeCalibrationModel::load(
    const std::string& path
) -> bool
{
    if (path.empty())
    {
        return false;
    }

    auto readModel = [](const std::filesystem::path& candidatePath,
                        GazeCalibrationModel& loadedModel) -> bool {
        std::ifstream input(candidatePath);

        if (!input)
        {
            return false;
        }

        std::string signature;
        std::uint32_t version = 0;
        std::size_t featureCount = 0;

        if (!(input >> signature >> version >> featureCount >> loadedModel.crossValidationError_) ||
            signature != PROFILE_SIGNATURE || version != PROFILE_VERSION ||
            featureCount != GAZE_FEATURE_COUNT ||
            !std::isfinite(loadedModel.crossValidationError_) ||
            loadedModel.crossValidationError_ < 0.0 ||
            loadedModel.crossValidationError_ > MAXIMUM_MEAN_CALIBRATION_ERROR)
        {
            return false;
        }

        for (auto& value : loadedModel.means_)
        {
            if (!(input >> value) || !std::isfinite(value) || std::abs(value) > 1.0e6)
            {
                return false;
            }
        }

        for (auto& value : loadedModel.scales_)
        {
            if (!(input >> value) || !std::isfinite(value) || value <= 0.0 || value > 1.0e6)
            {
                return false;
            }
        }

        for (auto& coordinate : loadedModel.coefficients_)
        {
            for (auto& value : coordinate)
            {
                if (!(input >> value) || !std::isfinite(value) || std::abs(value) > 1.0e6)
                {
                    return false;
                }
            }
        }

        std::string extraData;

        if (input >> extraData)
        {
            return false;
        }

        loadedModel.valid_ = true;
        return true;
    };

    std::filesystem::path const profilePath(path);
    auto temporaryPath = profilePath;
    temporaryPath += ".tmp";
    auto backupPath = profilePath;
    backupPath += ".bak";

    for (auto const& candidatePath : {profilePath, temporaryPath, backupPath})
    {
        GazeCalibrationModel loadedModel;

        if (readModel(candidatePath, loadedModel))
        {
            *this = loadedModel;
            return true;
        }
    }

    return false;
}
