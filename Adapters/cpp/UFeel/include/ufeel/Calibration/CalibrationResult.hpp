#ifndef UFEEL_CALIBRATION_CALIBRATIONRESULT_HPP
#define UFEEL_CALIBRATION_CALIBRATIONRESULT_HPP

#include "ufeel/Model/Calibration.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace ufeel
{

enum class CalibrationOutcome : std::uint8_t
{
    Succeeded,
    Failed,
    Cancelled,
};

struct CalibrationResult
{
    CalibrationOutcome outcome = CalibrationOutcome::Cancelled;
    bool profileSaved = false;
    std::optional<CalibrationDiagnostics> diagnostics;
    std::string message;

    [[nodiscard]] static auto success(bool profileSaved, bool hasProfileStore) -> CalibrationResult;
    [[nodiscard]] static auto failure(const CalibrationDiagnostics& diagnostics)
        -> CalibrationResult;
    [[nodiscard]] static auto cancelled() -> CalibrationResult;
};

} // namespace ufeel

#endif // UFEEL_CALIBRATION_CALIBRATIONRESULT_HPP
