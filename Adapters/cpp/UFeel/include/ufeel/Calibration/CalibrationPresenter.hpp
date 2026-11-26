#ifndef UFEEL_CALIBRATION_CALIBRATIONPRESENTER_HPP
#define UFEEL_CALIBRATION_CALIBRATIONPRESENTER_HPP

#include <string_view>

namespace ufeel
{

class CalibrationPresenter
{
public:

    virtual ~CalibrationPresenter() = default;

    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void showTarget(float x, float y) = 0;
    virtual void showInstruction(std::string_view text) = 0;
    virtual void showStatus(std::string_view text) = 0;

protected:

    CalibrationPresenter() = default;
    CalibrationPresenter(const CalibrationPresenter&) = default;
    CalibrationPresenter(CalibrationPresenter&&) = default;
    auto operator=(const CalibrationPresenter&) -> CalibrationPresenter& = default;
    auto operator=(CalibrationPresenter&&) -> CalibrationPresenter& = default;
};

} // namespace ufeel

#endif // UFEEL_CALIBRATION_CALIBRATIONPRESENTER_HPP
