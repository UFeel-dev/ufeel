#include "EyeTracking/GazeCalibrationModel.hpp"

#include "opencv2/core.hpp"
#include "opencv2/core/mat.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

namespace
{

constexpr std::size_t MINIMUM_RETAINED_SAMPLES_PER_TARGET = 8;
constexpr std::size_t SPATIAL_GRID_BAND_COUNT = 3;
constexpr double RIDGE_PENALTY = 0.08;
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

[[nodiscard]] auto robustSamples(
    const GazeCalibrationTarget& target
) -> std::vector<GazeFeatureVector>
{
    auto const& samples = target.samples;
    std::vector<GazeFeatureVector> retained;
    if (samples.empty())
    {
        return retained;
    }

    std::array<double, GAZE_FEATURE_COUNT> centers{};
    std::array<double, GAZE_FEATURE_COUNT> thresholds{};
    for (std::size_t feature = 0; feature < GAZE_FEATURE_COUNT; ++feature)
    {
        std::vector<double> values;
        values.reserve(samples.size());
        for (auto const& sample : samples)
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

    for (auto const& sample : samples)
    {
        bool inlier = true;
        for (std::size_t feature = 0; feature < GAZE_FEATURE_COUNT; ++feature)
        {
            if (!std::isfinite(sample[feature]) ||
                std::abs(sample[feature] - centers[feature]) > thresholds[feature])
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

[[nodiscard]] auto isFinite(
    const GazeFeatureVector& features
) -> bool
{
    return std::ranges::all_of(features, [](double value) { return std::isfinite(value); });
}

} // namespace

auto GazeCalibrationModel::fit(
    const std::vector<GazeCalibrationTarget>& targets
) -> std::optional<GazeCalibrationModel>
{
    std::vector<GazeCalibrationTarget> validTargets;
    validTargets.reserve(targets.size());
    for (auto const& target : targets)
    {
        if (!std::isfinite(target.target.x) || !std::isfinite(target.target.y) ||
            target.target.x < 0.0F || target.target.x > 1.0F || target.target.y < 0.0F ||
            target.target.y > 1.0F || target.samples.size() < GAZE_MINIMUM_SAMPLES_PER_TARGET ||
            robustSamples(target).size() < MINIMUM_RETAINED_SAMPLES_PER_TARGET)
        {
            continue;
        }
        validTargets.push_back(target);
    }
    if (validTargets.size() < GAZE_MINIMUM_CALIBRATION_TARGETS)
    {
        return std::nullopt;
    }

    auto const [minimumX, maximumX] =
        std::ranges::minmax_element(validTargets, {}, [](GazeCalibrationTarget const& target) {
            return target.target.x;
        });
    auto const [minimumY, maximumY] =
        std::ranges::minmax_element(validTargets, {}, [](GazeCalibrationTarget const& target) {
            return target.target.y;
        });
    if (maximumX->target.x - minimumX->target.x < 0.3F ||
        maximumY->target.y - minimumY->target.y < 0.3F)
    {
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
    if (std::ranges::any_of(horizontalBands, [](std::size_t count) { return count < 2; }) ||
        std::ranges::any_of(verticalBands, [](std::size_t count) { return count < 2; }))
    {
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
        auto heldOutSamples = robustSamples(validTargets[heldOut]);
        if (!trainingModel || heldOutSamples.empty())
        {
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
            return std::nullopt;
        }
        targetErrors.push_back(
            std::hypot(
                static_cast<double>(prediction->x - validTargets[heldOut].target.x),
                static_cast<double>(prediction->y - validTargets[heldOut].target.y)
            )
        );
    }

    double const meanError = std::accumulate(targetErrors.begin(), targetErrors.end(), 0.0) /
                             static_cast<double>(targetErrors.size());
    double const maximumError = *std::ranges::max_element(targetErrors);
    if (!std::isfinite(meanError) || meanError > MAXIMUM_MEAN_CALIBRATION_ERROR ||
        maximumError > MAXIMUM_TARGET_CALIBRATION_ERROR)
    {
        return std::nullopt;
    }

    auto model = fitRegression(validTargets);
    if (!model)
    {
        return std::nullopt;
    }
    model->crossValidationError_ = meanError;
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
    for (auto const& target : targets)
    {
        auto samples = robustSamples(target);
        if (samples.size() < MINIMUM_RETAINED_SAMPLES_PER_TARGET)
        {
            return std::nullopt;
        }
        samplesByTarget.push_back(std::move(samples));
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

    constexpr int COLUMN_COUNT = static_cast<int>(GAZE_FEATURE_COUNT + 1);
    std::size_t rowCount = 0;
    for (auto const& samples : samplesByTarget)
    {
        rowCount += samples.size();
    }
    cv::Mat design(static_cast<int>(rowCount), COLUMN_COUNT, CV_64F);
    cv::Mat targetValues(static_cast<int>(rowCount), 2, CV_64F);
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
                targets[targetIndex].target.x * balanceWeight;
            targetValues.at<double>(static_cast<int>(row), 1) =
                targets[targetIndex].target.y * balanceWeight;
            ++row;
        }
    }

    cv::Mat gram = design.t() * design;
    for (int column = 0; column < COLUMN_COUNT; ++column)
    {
        gram.at<double>(column, column) += column == 0 ? 1.0e-8 : RIDGE_PENALTY;
    }
    cv::Mat solution;
    if (!cv::solve(gram, design.t() * targetValues, solution, cv::DECOMP_SVD))
    {
        return std::nullopt;
    }

    for (int feature = 0; feature < COLUMN_COUNT; ++feature)
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
    return cv::Point2f(static_cast<float>(x), static_cast<float>(y));
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
    return static_cast<float>(std::exp(-2.5 * crossValidationError_));
}

auto GazeCalibrationModel::save(
    const std::string& path
) const -> bool
{
    if (!valid_ || path.empty())
    {
        return false;
    }

    std::filesystem::path const profilePath(path);
    auto temporaryPath = profilePath;
    temporaryPath += ".tmp";
    auto backupPath = profilePath;
    backupPath += ".bak";

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
    output.close();

    std::error_code error;
    bool const hasExistingProfile = std::filesystem::exists(profilePath, error);
    if (error)
    {
        std::filesystem::remove(temporaryPath, error);
        return false;
    }
    bool movedExistingProfile = false;
    if (hasExistingProfile)
    {
        if (std::filesystem::exists(backupPath, error) || error)
        {
            std::filesystem::remove(temporaryPath, error);
            return false;
        }
        std::filesystem::rename(profilePath, backupPath, error);
        if (error)
        {
            std::filesystem::remove(temporaryPath, error);
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
    if (movedExistingProfile)
    {
        std::filesystem::remove(backupPath, error);
    }
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
    std::ifstream input(path);
    if (!input)
    {
        return false;
    }

    std::string signature;
    std::uint32_t version = 0;
    std::size_t featureCount = 0;
    GazeCalibrationModel loadedModel;
    if (!(input >> signature >> version >> featureCount >> loadedModel.crossValidationError_) ||
        signature != PROFILE_SIGNATURE || version != PROFILE_VERSION ||
        featureCount != GAZE_FEATURE_COUNT || !std::isfinite(loadedModel.crossValidationError_) ||
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
    *this = loadedModel;
    return true;
}
