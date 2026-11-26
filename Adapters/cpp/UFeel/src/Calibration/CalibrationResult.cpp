#include "ufeel/Calibration/CalibrationResult.hpp"

#include "ufeel/Model/Calibration.hpp"
#include "ufeel/Text/Text.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace ufeel
{

namespace
{

auto successMessage(
    bool profileSaved, bool hasProfileStore
) -> std::string_view
{
    if (!hasProfileStore)
    {
        return "Eye tracking calibrated";
    }
    if (profileSaved)
    {
        return "Eye tracking calibrated and profile saved";
    }
    return "Eye tracking calibrated, but the profile could not be saved";
}

} // namespace

auto CalibrationResult::success(
    bool profileSaved, bool hasProfileStore
) -> CalibrationResult
{
    return {
        .outcome = CalibrationOutcome::Succeeded,
        .profileSaved = profileSaved,
        .diagnostics = std::nullopt,
        .message = std::string(successMessage(profileSaved, hasProfileStore)),
    };
}

auto CalibrationResult::failure(
    const CalibrationDiagnostics& diagnostics
) -> CalibrationResult
{
    return {
        .outcome = CalibrationOutcome::Failed,
        .profileSaved = false,
        .diagnostics = diagnostics,
        .message = text::calibrationFailure(diagnostics),
    };
}

auto CalibrationResult::cancelled() -> CalibrationResult
{
    return {
        .outcome = CalibrationOutcome::Cancelled,
        .profileSaved = false,
        .diagnostics = std::nullopt,
        .message = "Eye tracking calibration cancelled",
    };
}

} // namespace ufeel
