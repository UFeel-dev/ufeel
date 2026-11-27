#ifndef UFEEL_MODEL_STATUS_HPP
#define UFEEL_MODEL_STATUS_HPP

#include "ufeel.h"

#include <cstdint>
#include <string_view>

namespace ufeel
{

enum class Status : std::uint8_t
{
    Ok = UFEEL_STATUS_OK,
    NoEmotions = UFEEL_STATUS_NO_EMOTIONS,
    NoEyeTracking = UFEEL_STATUS_NO_EYE_TRACKING,
    NoSpeech = UFEEL_STATUS_NO_SPEECH,
    NoHeartRate = UFEEL_STATUS_NO_HEARTRATE,
    InvalidArgument = UFEEL_STATUS_INVALID_ARGUMENT,
    InvalidState = UFEEL_STATUS_INVALID_STATE,
    EyeTrackingCalibrationFailed = UFEEL_STATUS_EYE_TRACKING_CALIBRATION_FAILED,
    EyeTrackingProfileFailed = UFEEL_STATUS_EYE_TRACKING_PROFILE_FAILED,
    InternalError = UFEEL_STATUS_INTERNAL_ERROR,
    InvalidProcessor = UFEEL_STATUS_INVALID_PROCESSOR,
    InvalidFrame = UFEEL_STATUS_INVALID_FRAME,
    UnsupportedFeature = UFEEL_STATUS_UNSUPPORTED_FEATURE,
};

[[nodiscard]] auto toString(Status status) -> std::string_view;

[[nodiscard]] auto describe(Status status) -> std::string_view;

} // namespace ufeel

#endif // UFEEL_MODEL_STATUS_HPP
