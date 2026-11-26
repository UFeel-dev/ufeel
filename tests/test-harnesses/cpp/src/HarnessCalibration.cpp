#include "TestHarness/Harness.hpp"
#include "TestHarness/Ui.hpp"
#include "ufeel/Calibration/CalibrationResult.hpp"

#include <exception>
#include <raylib.h>
#include <string_view>

namespace
{

constexpr float targetRadius = 14.0F;
constexpr float instructionTop = 36.0F;
constexpr float instructionSize = 30.0F;

} // namespace

void Harness::show()
{
    calibrationVisible_ = true;
}

void Harness::hide()
{
    calibrationVisible_ = false;
}

void Harness::showTarget(
    float x, float y
)
{
    calibrationTarget_ = {.x = x, .y = y};
}

void Harness::showInstruction(
    std::string_view text
)
{
    calibrationInstruction_ = text;
}

void Harness::showStatus(
    std::string_view text
)
{
    calibrationStatus_ = text;
}

void Harness::toggleCalibration()
{
    if (calibration_->isRunning())
    {
        calibration_->cancel();
        return;
    }

    diagnostics_ = "No calibration diagnostics yet.";
    calibration_->start();
}

void Harness::updateCalibration()
{
    try
    {
        const auto result = calibration_->update();
        if (!result)
        {
            return;
        }

        if (result->outcome == ufeel::CalibrationOutcome::Succeeded)
        {
            diagnostics_ = "Calibration completed successfully.";
        }
        else if (result->diagnostics)
        {
            diagnostics_ = result->message;
        }
        idleStatus_ = result->message;
    }
    catch (const std::exception& error)
    {
        idleStatus_ = error.what();
    }
}

void Harness::drawCalibrationOverlay() const
{
    const auto width = static_cast<float>(GetScreenWidth());
    const auto height = static_cast<float>(GetScreenHeight());

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ui::color::overlay);
    ui::marker(
        {.x = calibrationTarget_.x * width, .y = calibrationTarget_.y * height},
        targetRadius,
        ui::color::calibrationTarget,
        ui::color::lightText
    );
    ui::centeredText(
        calibrationInstruction_, instructionTop, instructionSize, ui::color::lightText
    );
    ui::centeredText(
        calibrationStatus_,
        instructionTop + instructionSize + ui::spacing,
        ui::fontSize,
        ui::color::border
    );
}
