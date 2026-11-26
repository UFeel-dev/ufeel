#ifndef UFEEL_MODEL_EYETRACKINGRESULT_HPP
#define UFEEL_MODEL_EYETRACKINGRESULT_HPP

#include "ufeel.h"
#include "ufeel/Model/Flags.hpp"

#include <cstdint>
#include <type_traits>

namespace ufeel
{

enum class EyeTrackingStatus : std::uint8_t
{
    NotCalibrated = UFEEL_GAZE_STATUS_NOT_CALIBRATED,
    FaceUnavailable = UFEEL_GAZE_STATUS_FACE_UNAVAILABLE,
    LowQuality = UFEEL_GAZE_STATUS_LOW_QUALITY,
    Calibrating = UFEEL_GAZE_STATUS_CALIBRATING,
    Valid = UFEEL_GAZE_STATUS_VALID,
    Disabled = UFEEL_GAZE_STATUS_DISABLED,
    FrameUnavailable = UFEEL_GAZE_STATUS_FRAME_UNAVAILABLE,
    InvalidProcessor = UFEEL_GAZE_STATUS_INVALID_PROCESSOR,
    InternalError = UFEEL_GAZE_STATUS_INTERNAL_ERROR,
};

enum class EyeTrackingDirection : std::uint8_t
{
    None = UFEEL_GAZE_DIRECTION_NONE,
    Left = UFEEL_GAZE_DIRECTION_LEFT,
    Right = UFEEL_GAZE_DIRECTION_RIGHT,
    Up = UFEEL_GAZE_DIRECTION_UP,
    Down = UFEEL_GAZE_DIRECTION_DOWN,
    Center = UFEEL_GAZE_DIRECTION_CENTER,
};

template <> struct EnableFlags<EyeTrackingDirection> : std::true_type
{
};

enum class EyeState : std::uint8_t
{
    None = UFEEL_EYE_STATE_NONE,
    Tracked = UFEEL_EYE_STATE_TRACKED,
    LeftClosed = UFEEL_EYE_STATE_LEFT_CLOSED,
    RightClosed = UFEEL_EYE_STATE_RIGHT_CLOSED,
};

template <> struct EnableFlags<EyeState> : std::true_type
{
};

struct EyeTrackingResult
{
    EyeTrackingStatus status = EyeTrackingStatus::FrameUnavailable;
    float estimatedX = 0.0F;
    float estimatedY = 0.0F;
    float recordedX = 0.0F;
    float recordedY = 0.0F;
    float confidence = 0.0F;
    EyeTrackingDirection directions = EyeTrackingDirection::None;
    float gazeX = 0.0F;
    float gazeY = 0.0F;
    float leftOpenness = 0.0F;
    float rightOpenness = 0.0F;
    EyeState eyeState = EyeState::None;
    std::uint32_t blinkCount = 0;
    float eyesClosedSeconds = 0.0F;

    [[nodiscard]] auto isValid() const -> bool
    {
        return status == EyeTrackingStatus::Valid;
    }

    [[nodiscard]] auto isTracked() const -> bool
    {
        return hasAll(eyeState, EyeState::Tracked);
    }

    [[nodiscard]] auto areEyesClosed() const -> bool
    {
        return hasAll(eyeState, EyeState::LeftClosed | EyeState::RightClosed);
    }
};

} // namespace ufeel

#endif // UFEEL_MODEL_EYETRACKINGRESULT_HPP
