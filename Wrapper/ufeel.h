// NOLINTBEGIN

#ifndef UFEEL_H
#define UFEEL_H

#include <stdint.h>

#if defined(_WIN32)
#if defined(UFEEL_BUILDING_LIBRARY)
#define UFEEL_API __declspec(dllexport)
#else
#define UFEEL_API __declspec(dllimport)
#endif
#else
#define UFEEL_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UFeelProcessor UFeelProcessor;

/** An image frame returned by the library. Release it with ufeel_free_frame(). */
typedef struct
{
    uint8_t* data;
    int32_t width;
    int32_t height;
    int32_t channels;
    int32_t stride;
} UFeelFrame;

/** One named emotion and its computed score. Release the array with ufeel_free_emotions(). */
typedef struct
{
    char* name;
    float value;
} UFeelPair;

typedef struct UFeelGaze
{
    float x;
    float y;
    uint32_t directions;
    uint32_t valid;
} UFeelGaze;

typedef enum
{
    UFEEL_HEARTRATE_STATUS_JSON_PARSE_ERROR = 0,
    UFEEL_HEARTRATE_STATUS_JSON_PARSE_OUT_OF_RANGE = 1,
    UFEEL_HEARTRATE_STATUS_REQUEST_FAILED = 2,
    UFEEL_HEARTRATE_STATUS_NULL_PROCESSOR = 3
} UFeelHeartRateStatus;

typedef struct
{
        int32_t status;
        uint32_t bpm;
} UFeelHeartRateResult;

typedef struct UFeelGaze
{
    UFEEL_GAZE_STATUS_NOT_CALIBRATED = 0,
    UFEEL_GAZE_STATUS_FACE_UNAVAILABLE = 1,
    UFEEL_GAZE_STATUS_BLINKING = 2,
    UFEEL_GAZE_STATUS_LOW_QUALITY = 3,
    UFEEL_GAZE_STATUS_CALIBRATING = 4,
    UFEEL_GAZE_STATUS_VALID = 5,
    UFEEL_GAZE_STATUS_DISABLED = 6,
    UFEEL_GAZE_STATUS_FRAME_UNAVAILABLE = 7
} UFeelGazeStatus;

/** Bit flags indicating the estimated gaze direction. Multiple flags may be set. */
typedef enum
{
    UFEEL_GAZE_DIRECTION_NONE = 0,
    UFEEL_GAZE_DIRECTION_LEFT = 1 << 0,
    UFEEL_GAZE_DIRECTION_RIGHT = 1 << 1,
    UFEEL_GAZE_DIRECTION_UP = 1 << 2,
    UFEEL_GAZE_DIRECTION_DOWN = 1 << 3,
    UFEEL_GAZE_DIRECTION_CENTER = 1 << 4
} UFeelGazeDirection;

/** Estimated gaze coordinates and diagnostics for the current video frame. */
typedef struct
{
    int32_t status;
    float x;
    float y;
    uint32_t directions;
    uint32_t valid;
} UFeelGaze;

typedef struct UFeelGazeEx
{
    UFEEL_GAZE_STATUS_NOT_CALIBRATED = 0,
    UFEEL_GAZE_STATUS_FACE_UNAVAILABLE = 1,
    UFEEL_GAZE_STATUS_BLINKING = 2,
    UFEEL_GAZE_STATUS_LOW_QUALITY = 3,
    UFEEL_GAZE_STATUS_CALIBRATING = 4,
    UFEEL_GAZE_STATUS_VALID = 5,
    UFEEL_GAZE_STATUS_DISABLED = 6,
    UFEEL_GAZE_STATUS_FRAME_UNAVAILABLE = 7
} UFeelGazeStatus;

/** Bit flags indicating the estimated gaze direction. Multiple flags may be set. */
typedef enum
{
    UFEEL_GAZE_DIRECTION_NONE = 0,
    UFEEL_GAZE_DIRECTION_LEFT = 1 << 0,
    UFEEL_GAZE_DIRECTION_RIGHT = 1 << 1,
    UFEEL_GAZE_DIRECTION_UP = 1 << 2,
    UFEEL_GAZE_DIRECTION_DOWN = 1 << 3,
    UFEEL_GAZE_DIRECTION_CENTER = 1 << 4
} UFeelGazeDirection;

/** Estimated gaze coordinates and diagnostics for the current video frame. */
typedef struct
{
    int32_t status;
    float x;
    float y;
    float recordedX;
    float recordedY;
    float confidence;
    int32_t directions;
} UFeelGazeResult;

/** Status returned by functions that can fail. */
typedef enum
{
    UFEEL_STATUS_OK = 0,
    UFEEL_STATUS_NO_GAZE = 1,
    UFEEL_STATUS_NO_HEARTRATE = 2,
    UFEEL_STATUS_INVALID_ARGUMENT = 3,
    UFEEL_STATUS_INVALID_STATE = 4,
    UFEEL_STATUS_EYE_TRACKING_CALIBRATION_FAILED = 5,
    UFEEL_STATUS_EYE_TRACKING_PROFILE_FAILED = 6,
    UFEEL_STATUS_INTERNAL_ERROR = 7
} UFeelStatus;

/** Progress of the active eye-tracking calibration session. */
typedef struct
{
    int32_t targetsWithEnoughSamples;
    int32_t targetCount;
    int32_t samplesForCurrentTarget;
    int32_t requiredSamplesPerTarget;
} UFeelEyeTrackingCalibrationProgress;

/** Diagnostic measurements from the most recent eye-tracking calibration. */
typedef struct
{
    int32_t failureReason;
    int32_t inputTargetCount;
    int32_t targetsWithEnoughRawSamples;
    int32_t targetsWithEnoughRetainedSamples;
    int32_t validTargetCount;
    int32_t minimumRetainedSamples;
    int32_t maximumRetainedSamples;
    float horizontalSpan;
    float verticalSpan;
    float meanCrossValidationError;
    float maximumCrossValidationError;
    int32_t crossValidationTargetsEvaluated;
    int32_t meanErrorExceeded;
    int32_t maximumErrorExceeded;
    int32_t regressionFailures;
    int32_t invalidTargetCount;
    int32_t targetsWithInsufficientRawSamples;
    int32_t targetsWithInsufficientRetainedSamples;
} UFeelEyeTrackingCalibrationDiagnostics;

typedef enum
{
    UFEEL_HEARTRATE_STATUS_JSON_PARSE_ERROR = 0,
    UFEEL_HEARTRATE_STATUS_JSON_PARSE_OUT_OF_RANGE = 1,
    UFEEL_HEARTRATE_STATUS_REQUEST_FAILED = 2,
    UFEEL_HEARTRATE_STATUS_NULL_PROCESSOR = 3
} UFeelHeartRateStatus;

typedef struct
{
    int32_t status;
    uint32_t bpm;
} UFeelHeartRateResult;

/** Creates a processor. Returns NULL if initialization fails. */
UFEEL_API UFeelProcessor*
ufeel_create(const char* utf8ModelDirectory, const char* utf8SpeechModelPath);

/** Destroys a processor created by ufeel_create(). Accepts NULL. */
UFEEL_API void ufeel_destroy(UFeelProcessor* processor);

/** Processes the next input frame. Returns 1 when processing succeeds, otherwise 0. */
UFEEL_API int32_t ufeel_update(UFeelProcessor* processor);

/** Returns a copy of the latest processed image, or NULL if no frame is available. */
UFEEL_API UFeelFrame* ufeel_get_frame(UFeelProcessor* processor);

/** Releases a frame returned by ufeel_get_frame(). Accepts NULL. */
UFEEL_API void ufeel_free_frame(UFeelFrame* frame);

/** Returns the current emotion scores. Set *count to zero if no scores are available. */
UFEEL_API UFeelPair* ufeel_get_emotions(
    UFeelProcessor* processor, int32_t* count
); // TODO: update to match eye tracking and heartrate

/** Releases emotion results returned by ufeel_get_emotions(). Accepts NULL. */
UFEEL_API void ufeel_free_emotions(UFeelPair* emotions, int32_t count);

/** Begins eye-tracking calibration. */
UFEEL_API int32_t ufeel_begin_eye_tracking_calibration(UFeelProcessor* processor);

/** Sets the screen coordinate currently being used as the eye-tracking calibration target. */
UFEEL_API int32_t
ufeel_set_eye_tracking_calibration_target(UFeelProcessor* processor, float screenX, float screenY);

/** Fits and activates the eye-tracking calibration model. */
UFEEL_API int32_t ufeel_finish_eye_tracking_calibration(UFeelProcessor* processor);

/** Aborts eye-tracking calibration without activating the unfinished model. */
UFEEL_API int32_t ufeel_cancel_eye_tracking_calibration(UFeelProcessor* processor);

/** Retrieves the latest gaze estimate and its validity status. */
UFEEL_API int32_t ufeel_get_gaze_result(UFeelProcessor* processor, UFeelGazeResult* result);

/** Retrieves progress for the current eye-tracking calibration session. */
UFEEL_API int32_t ufeel_get_eye_tracking_calibration_progress(
    UFeelProcessor* processor, UFeelEyeTrackingCalibrationProgress* progress
);

/** Retrieves diagnostic measurements from eye-tracking calibration. */
UFEEL_API int32_t ufeel_get_eye_tracking_calibration_diagnostics(
    UFeelProcessor* processor, UFeelEyeTrackingCalibrationDiagnostics* diagnostics
);

/** Saves the active eye-tracking calibration profile to a UTF-8 file path. */
UFEEL_API int32_t
ufeel_save_eye_tracking_calibration_profile(UFeelProcessor* processor, const char* utf8Path);

/** Loads an eye-tracking calibration profile from a UTF-8 file path. */
UFEEL_API int32_t
ufeel_load_eye_tracking_calibration_profile(UFeelProcessor* processor, const char* utf8Path);

/** Enables or disables speech-to-text processing. */
UFEEL_API int32_t ufeel_set_speech_to_text_enabled(UFeelProcessor* processor, int32_t enabled);

/** Returns the latest speech-to-text result as a library-allocated UTF-8 string. */
UFEEL_API char* ufeel_get_speech_to_text(
    UFeelProcessor* processor
); // TODO: update to match eye tracking and heartrate

/** Releases a string returned by ufeel_get_speech_to_text(). Accepts NULL. */
UFEEL_API void ufeel_free_string(char* value);

bool ufeel_get_heartrate(void* processor, UFeelHeartRateResult* result);

#ifdef __cplusplus
}
#endif

#endif

// NOLINTEND
