#include "ufeel/Calibration/CalibrationResult.hpp"
#include "ufeel/Model/Calibration.hpp"

#include <gtest/gtest.h>
#include <string>

TEST(
    CalibrationResultTest, DescribesSuccesses
)
{
    const auto withoutStore = ufeel::CalibrationResult::success(false, false);
    const auto saved = ufeel::CalibrationResult::success(true, true);
    const auto unsaved = ufeel::CalibrationResult::success(false, true);

    EXPECT_EQ(withoutStore.outcome, ufeel::CalibrationOutcome::Succeeded);
    EXPECT_EQ(withoutStore.message, "Eye tracking calibrated");
    EXPECT_TRUE(saved.profileSaved);
    EXPECT_EQ(saved.message, "Eye tracking calibrated and profile saved");
    EXPECT_EQ(unsaved.message, "Eye tracking calibrated, but the profile could not be saved");
}

TEST(
    CalibrationResultTest, KeepsFailureDiagnostics
)
{
    const ufeel::CalibrationDiagnostics diagnostics{
        .failureReason = ufeel::CalibrationFailureReason::RegressionFailed,
    };

    const auto result = ufeel::CalibrationResult::failure(diagnostics);

    EXPECT_EQ(result.outcome, ufeel::CalibrationOutcome::Failed);
    if (!result.diagnostics.has_value())
    {
        FAIL() << "a failure keeps its diagnostics";
    }
    EXPECT_EQ(result.diagnostics->failureReason, ufeel::CalibrationFailureReason::RegressionFailed);
    EXPECT_TRUE(result.message.starts_with(
        "Failure: The calibration model could not be fitted. Repeat calibration."
    ));
}

TEST(
    CalibrationResultTest, DescribesCancellations
)
{
    const auto result = ufeel::CalibrationResult::cancelled();

    EXPECT_EQ(result.outcome, ufeel::CalibrationOutcome::Cancelled);
    EXPECT_FALSE(result.diagnostics.has_value());
    EXPECT_EQ(result.message, "Eye tracking calibration cancelled");
}
