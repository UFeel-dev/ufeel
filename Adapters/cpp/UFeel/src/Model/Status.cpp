#include "ufeel/Model/Status.hpp"

#include <string_view>

namespace ufeel
{

auto toString(
    Status status
) -> std::string_view
{
    switch (status)
    {
    case Status::Ok:
        return "Ok";
    case Status::NoEmotions:
        return "NoEmotions";
    case Status::NoEyeTracking:
        return "NoEyeTracking";
    case Status::NoSpeech:
        return "NoSpeech";
    case Status::NoHeartRate:
        return "NoHeartRate";
    case Status::InvalidArgument:
        return "InvalidArgument";
    case Status::InvalidState:
        return "InvalidState";
    case Status::EyeTrackingCalibrationFailed:
        return "EyeTrackingCalibrationFailed";
    case Status::EyeTrackingProfileFailed:
        return "EyeTrackingProfileFailed";
    case Status::InternalError:
        return "InternalError";
    case Status::InvalidProcessor:
        return "InvalidProcessor";
    case Status::InvalidFrame:
        return "InvalidFrame";
    case Status::UnsupportedFeature:
        return "UnsupportedFeature";
    }

    return "Unknown";
}

auto describe(
    Status status
) -> std::string_view
{
    switch (status)
    {
    case Status::Ok:
        return "The operation succeeded.";
    case Status::NoEmotions:
        return "No emotion scores are available.";
    case Status::NoEyeTracking:
        return "A valid gaze estimate is not available.";
    case Status::NoSpeech:
        return "No speech transcription is available.";
    case Status::NoHeartRate:
        return "No heart-rate reading is available.";
    case Status::InvalidArgument:
        return "One or more arguments are invalid.";
    case Status::InvalidState:
        return "The processor is not in a state that permits this operation.";
    case Status::EyeTrackingCalibrationFailed:
        return "Eye-tracking calibration failed.";
    case Status::EyeTrackingProfileFailed:
        return "The eye-tracking calibration profile operation failed.";
    case Status::InternalError:
        return "An internal native-library error occurred.";
    case Status::InvalidProcessor:
        return "The processor handle is invalid.";
    case Status::InvalidFrame:
        return "The frame is unavailable or invalid.";
    case Status::UnsupportedFeature:
        return "The feature is not built into this library.";
    }

    return "An unknown native-library error occurred.";
}

} // namespace ufeel
