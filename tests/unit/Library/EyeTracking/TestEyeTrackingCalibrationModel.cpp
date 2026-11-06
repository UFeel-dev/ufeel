#include "EyeTracking/EyeTrackingCalibrationModel.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <ios>
#include <limits>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace
{

auto makeGridTargets(
    float minimumX = 0.1F,
    float middleX = 0.5F,
    float maximumX = 0.9F,
    float minimumY = 0.1F,
    float middleY = 0.5F,
    float maximumY = 0.9F
) -> std::vector<EyeTrackingCalibrationTarget>
{
    const std::array<float, 3> xs{minimumX, middleX, maximumX};
    const std::array<float, 3> ys{minimumY, middleY, maximumY};
    std::vector<EyeTrackingCalibrationTarget> targets;

    for (const auto y : ys)
    {
        for (const auto x : xs)
        {
            EyeTrackingCalibrationTarget target{.target = {x, y}};
            for (std::size_t sampleIndex = 0; sampleIndex < GAZE_MINIMUM_SAMPLES_PER_TARGET + 1;
                 ++sampleIndex)
            {
                EyeTrackingFeatureVector features{};
                const double variation = static_cast<double>(sampleIndex) * 0.0001;
                features.at(0) = static_cast<double>(x) + variation;
                features.at(1) = static_cast<double>(y) - variation;
                target.samples.push_back(features);
            }
            auto outlier = target.samples.front();
            outlier.at(0) += 1000.0;
            target.samples.push_back(outlier);
            auto invalidSample = target.samples.front();
            invalidSample.at(0) = std::numeric_limits<double>::quiet_NaN();
            target.samples.push_back(invalidSample);
            targets.push_back(std::move(target));
        }
    }

    return targets;
}

class EyeTrackingCalibrationModelTest : public ::testing::Test
{
protected:

    static auto profilePath() -> std::filesystem::path
    {
        return std::filesystem::temp_directory_path() / "ufeel-eye-tracking-profile-test.dat";
    }

    void TearDown() override
    {
        std::error_code error;
        std::filesystem::remove(profilePath(), error);
        std::filesystem::remove(profilePath().string() + ".tmp", error);
        std::filesystem::remove(profilePath().string() + ".bak", error);
    }
};

} // namespace

TEST_F(
    EyeTrackingCalibrationModelTest, RejectsTooFewTargetsAndReportsDiagnostics
)
{
    CalibrationDiagnostics diagnostics;
    const auto model = EyeTrackingCalibrationModel::fit({}, &diagnostics);

    EXPECT_FALSE(model);
    EXPECT_EQ(diagnostics.failureReason, CalibrationFailureReason::TooFewValidTargets);
    EXPECT_EQ(diagnostics.inputTargetCount, 0U);
    EXPECT_EQ(diagnostics.minimumRetainedSamples, 0U);
}

TEST_F(
    EyeTrackingCalibrationModelTest, RejectsInvalidTargetsAndInsufficientSamples
)
{
    auto targets = makeGridTargets();
    targets.front().target.x = std::numeric_limits<float>::quiet_NaN();
    targets.back().samples.clear();

    CalibrationDiagnostics diagnostics;
    EXPECT_FALSE(EyeTrackingCalibrationModel::fit(targets, &diagnostics));
    EXPECT_EQ(diagnostics.invalidTargetCount, 1U);
    EXPECT_EQ(diagnostics.targetsWithInsufficientRawSamples, 1U);
    EXPECT_EQ(diagnostics.failureReason, CalibrationFailureReason::InsufficientSamples);
}

TEST_F(
    EyeTrackingCalibrationModelTest, RejectsInsufficientHorizontalCoverage
)
{
    const auto targets = makeGridTargets(0.4F, 0.5F, 0.6F);
    CalibrationDiagnostics diagnostics;

    EXPECT_FALSE(EyeTrackingCalibrationModel::fit(targets, &diagnostics));
    EXPECT_EQ(diagnostics.failureReason, CalibrationFailureReason::InsufficientHorizontalCoverage);
}

TEST_F(
    EyeTrackingCalibrationModelTest, RejectsInsufficientVerticalCoverage
)
{
    const auto targets = makeGridTargets(0.1F, 0.5F, 0.9F, 0.4F, 0.5F, 0.6F);
    CalibrationDiagnostics diagnostics;

    EXPECT_FALSE(EyeTrackingCalibrationModel::fit(targets, &diagnostics));
    EXPECT_EQ(diagnostics.failureReason, CalibrationFailureReason::InsufficientVerticalCoverage);
}

TEST_F(
    EyeTrackingCalibrationModelTest, RejectsInsufficientSpatialGridCoverage
)
{
    auto targets = makeGridTargets();
    std::erase_if(targets, [](const EyeTrackingCalibrationTarget& target) {
        return target.target.x == 0.1F && (target.target.y == 0.1F || target.target.y == 0.5F);
    });
    auto additionalBottomTarget = targets.front();
    additionalBottomTarget.target = {0.6F, 0.1F};
    targets.push_back(std::move(additionalBottomTarget));
    auto additionalMiddleTarget = targets.front();
    additionalMiddleTarget.target = {0.6F, 0.5F};
    targets.push_back(std::move(additionalMiddleTarget));
    CalibrationDiagnostics diagnostics;

    EXPECT_FALSE(EyeTrackingCalibrationModel::fit(targets, &diagnostics));
    EXPECT_EQ(diagnostics.failureReason, CalibrationFailureReason::InsufficientSpatialGridCoverage);
}

TEST_F(
    EyeTrackingCalibrationModelTest, RejectsCalibrationWithUnpredictiveSamples
)
{
    auto targets = makeGridTargets();
    for (auto& target : targets)
    {
        for (auto& sample : target.samples)
        {
            sample.fill(0.0);
        }
    }

    CalibrationDiagnostics diagnostics;
    EXPECT_FALSE(EyeTrackingCalibrationModel::fit(targets, &diagnostics));
    EXPECT_TRUE(diagnostics.meanErrorExceeded);
    EXPECT_EQ(
        diagnostics.failureReason, CalibrationFailureReason::CrossValidationMeanErrorExceeded
    );
}

TEST_F(
    EyeTrackingCalibrationModelTest, FitsPredictsAndRoundTripsCalibrationProfile
)
{
    auto targets = makeGridTargets();
    CalibrationDiagnostics diagnostics;
    auto model = EyeTrackingCalibrationModel::fit(targets, &diagnostics);

    ASSERT_TRUE(model);
    EXPECT_TRUE(model->isValid());
    EXPECT_EQ(diagnostics.failureReason, CalibrationFailureReason::None);
    EXPECT_EQ(diagnostics.crossValidationTargetsEvaluated, targets.size());
    EXPECT_GE(model->confidence(), 0.0F);
    EXPECT_LE(model->confidence(), 1.0F);
    EXPECT_EQ(diagnostics.minimumRetainedSamples, GAZE_MINIMUM_SAMPLES_PER_TARGET + 1);

    EyeTrackingFeatureVector features{};
    features.at(0) = 0.5;
    features.at(1) = 0.5;
    const auto prediction = model->predict(features);
    ASSERT_TRUE(prediction);
    EXPECT_NEAR(prediction->x, 0.5F, 0.05F);
    EXPECT_NEAR(prediction->y, 0.5F, 0.05F);

    EXPECT_TRUE(model->save(profilePath().string()));
    EyeTrackingCalibrationModel loaded;
    ASSERT_TRUE(loaded.load(profilePath().string()));
    EXPECT_TRUE(loaded.isValid());
    const auto loadedPrediction = loaded.predict(features);
    ASSERT_TRUE(loadedPrediction);
    EXPECT_NEAR(loadedPrediction->x, prediction->x, 1.0e-5F);
    EXPECT_NEAR(loadedPrediction->y, prediction->y, 1.0e-5F);
}

TEST_F(
    EyeTrackingCalibrationModelTest, LoadsValidTemporaryOrBackupProfiles
)
{
    auto model = EyeTrackingCalibrationModel::fit(makeGridTargets());
    ASSERT_TRUE(model);
    ASSERT_TRUE(model->save(profilePath().string()));

    const auto temporaryPath = std::filesystem::path(profilePath().string() + ".tmp");
    const auto backupPath = std::filesystem::path(profilePath().string() + ".bak");
    std::filesystem::copy_file(profilePath(), temporaryPath);
    {
        std::ofstream corrupt(profilePath(), std::ios::trunc);
        corrupt << "invalid profile\n";
    }

    EyeTrackingCalibrationModel loadedFromTemporary;
    ASSERT_TRUE(loadedFromTemporary.load(profilePath().string()));
    EXPECT_TRUE(loadedFromTemporary.isValid());

    std::filesystem::remove(temporaryPath);
    ASSERT_TRUE(model->save(profilePath().string()));
    std::filesystem::copy_file(profilePath(), backupPath);
    {
        std::ofstream corrupt(profilePath(), std::ios::trunc);
        corrupt << "invalid profile\n";
    }
    EyeTrackingCalibrationModel loadedFromBackup;
    EXPECT_TRUE(loadedFromBackup.load(profilePath().string()));
    EXPECT_TRUE(loadedFromBackup.isValid());
}

TEST_F(
    EyeTrackingCalibrationModelTest, RejectsUnwritableAndMalformedProfiles
)
{
    auto model = EyeTrackingCalibrationModel::fit(makeGridTargets());
    ASSERT_TRUE(model);
    EXPECT_FALSE(model->save((profilePath() / "profile.dat").string()));

    {
        std::ofstream malformed(profilePath(), std::ios::trunc);
        ASSERT_TRUE(malformed);
        malformed << "UFEEL_GAZE_PROFILE 999 " << GAZE_FEATURE_COUNT << " 0.1\n";
    }

    EyeTrackingCalibrationModel loaded;
    EXPECT_FALSE(loaded.load(profilePath().string()));
    EXPECT_FALSE(loaded.isValid());
}

TEST_F(
    EyeTrackingCalibrationModelTest, RejectsInvalidPredictionAndProfileInputs
)
{
    EyeTrackingCalibrationModel model;
    EyeTrackingFeatureVector features{};

    EXPECT_FALSE(model.isValid());
    EXPECT_FLOAT_EQ(model.confidence(), 0.0F);
    EXPECT_FALSE(model.predict(features));
    EXPECT_FALSE(model.save(""));
    EXPECT_FALSE(model.save(profilePath().string()));
    EXPECT_FALSE(model.load(""));
    EXPECT_FALSE(model.load(profilePath().string()));

    auto targets = makeGridTargets();
    auto fitted = EyeTrackingCalibrationModel::fit(targets);
    ASSERT_TRUE(fitted);
    features.at(0) = std::numeric_limits<double>::infinity();
    EXPECT_FALSE(fitted->predict(features));
}

TEST_F(
    EyeTrackingCalibrationModelTest, ConvertsEveryFailureReasonToText
)
{
    const std::array reasons{
        CalibrationFailureReason::None,
        CalibrationFailureReason::NotCalibrating,
        CalibrationFailureReason::TooFewValidTargets,
        CalibrationFailureReason::InsufficientSamples,
        CalibrationFailureReason::InsufficientHorizontalCoverage,
        CalibrationFailureReason::InsufficientVerticalCoverage,
        CalibrationFailureReason::InsufficientSpatialGridCoverage,
        CalibrationFailureReason::RegressionFailed,
        CalibrationFailureReason::CrossValidationMeanErrorExceeded,
        CalibrationFailureReason::CrossValidationMaxErrorExceeded,
        CalibrationFailureReason::InvalidCrossValidationResult,
    };

    for (const auto reason : reasons)
    {
        EXPECT_NE(calibrationFailureReasonToString(reason), nullptr);
        EXPECT_NE(std::string(calibrationFailureReasonToString(reason)).size(), 0U);
    }

    EXPECT_STREQ(
        calibrationFailureReasonToString(static_cast<CalibrationFailureReason>(255)),
        "Unknown calibration failure"
    );
}
