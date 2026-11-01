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

/** Status returned by functions that can fail. */
typedef enum
{
    UFEEL_STATUS_OK = 0,
    UFEEL_STATUS_NO_EMOTIONS = 1,
    UFEEL_STATUS_NO_GAZE = 2,
    UFEEL_STATUS_NO_SPEECH = 3,
    UFEEL_STATUS_NO_HEARTRATE = 4,
    UFEEL_STATUS_INVALID_ARGUMENT = 5,
    UFEEL_STATUS_INVALID_STATE = 6,
    UFEEL_STATUS_EYE_TRACKING_CALIBRATION_FAILED = 7,
    UFEEL_STATUS_EYE_TRACKING_PROFILE_FAILED = 8,
    UFEEL_STATUS_INTERNAL_ERROR = 9,
    UFEEL_STATUS_INVALID_PROCESSOR = 10,
    UFEEL_STATUS_INVALID_FRAME = 11,
} UFeelStatus;

//? EMOTIONS

/** One named emotion and its computed score. Release the array with ufeel_free_emotions(). */
typedef struct
{
    char* name;
    float value;
} UFeelPair;

/** Status describing whether a emotions detection is valid. */
typedef enum
{
    UFEEL_EMOTIONS_STATUS_FACE_UNAVAILABLE = 0,
    UFEEL_EMOTIONS_STATUS_VALID = 1,
    UFEEL_EMOTIONS_STATUS_FRAME_UNAVAILABLE = 2
} UFeelEmotionsStatus;

/** Estimated emotions and diagnostics for the current video frame. */
typedef struct
{
    int32_t status;
    int32_t count;
    UFeelPair* emotions;
} UFeelEmotionsResult;

//? GAZE

/** Status describing whether a eye_tracking estimate is currently usable. */
typedef enum
{
    UFEEL_GAZE_STATUS_NOT_CALIBRATED = 0,
    UFEEL_GAZE_STATUS_FACE_UNAVAILABLE = 1,
    UFEEL_GAZE_STATUS_BLINKING = 2,
    UFEEL_GAZE_STATUS_LOW_QUALITY = 3,
    UFEEL_GAZE_STATUS_CALIBRATING = 4,
    UFEEL_GAZE_STATUS_VALID = 5,
    UFEEL_GAZE_STATUS_DISABLED = 6,
    UFEEL_GAZE_STATUS_FRAME_UNAVAILABLE = 7
} UFeelEyeTrackingStatus;

/** Bit flags indicating the estimated eye_tracking direction. Multiple flags may be set. */
typedef enum
{
    UFEEL_GAZE_DIRECTION_NONE = 0,
    UFEEL_GAZE_DIRECTION_LEFT = 1 << 0,
    UFEEL_GAZE_DIRECTION_RIGHT = 1 << 1,
    UFEEL_GAZE_DIRECTION_UP = 1 << 2,
    UFEEL_GAZE_DIRECTION_DOWN = 1 << 3,
    UFEEL_GAZE_DIRECTION_CENTER = 1 << 4
} UFeelEyeTrackingDirection;

/** Estimated eye_tracking coordinates and diagnostics for the current video frame. */
typedef struct
{
    int32_t status;
    float x;
    float y;
    float recordedX;
    float recordedY;
    float confidence;
    int32_t directions;
} UFeelEyeTrackingResult;

/** Progress of the active eye tracking calibration session. */
typedef struct
{
    int32_t targetsWithEnoughSamples;
    int32_t targetCount;
    int32_t samplesForCurrentTarget;
    int32_t requiredSamplesPerTarget;
} UFeelEyeTrackingCalibrationProgress;

/** Diagnostic measurements from the most recent eye tracking calibration. */
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

//? SPEECH

typedef enum
{
    UFEEL_SPEECH_STATUS_FACE_UNAVAILABLE = 0,
    UFEEL_SPEECH_STATUS_VALID = 1,
    UFEEL_SPEECH_STATUS_FRAME_UNAVAILABLE = 2
} UFeelSpeechStatus;

typedef struct
{
    int32_t status;
    char* speech;
} UFeelSpeechResult;

//? HEARTRATE

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
UFEEL_API UFeelProcessor* ufeel_create(const char* utf8ModelDirectory, const char* utf8SpeechModelPath);

/** Destroys a processor created by ufeel_create(). Accepts NULL. */
UFEEL_API void ufeel_destroy(UFeelProcessor* processor);

/** Processes the next input frame. Returns 1 when processing succeeds, otherwise 0. */
UFEEL_API int32_t ufeel_update(UFeelProcessor* processor, int32_t detectionType);

/** Processes the next input frame. Returns 1 when processing succeeds, otherwise 0. */
UFEEL_API int32_t ufeel_update_from_frame(UFeelProcessor* processor, int32_t detectionType, const UFeelFrame* frame);

/** Processes the next input frame. Returns 1 when processing succeeds, otherwise 0. */
UFEEL_API int32_t ufeel_update_frame(UFeelProcessor* processor);

/** Set the lasted processed image to the one you gave. */
UFEEL_API int32_t ufeel_set_frame(UFeelProcessor* processor, const  UFeelFrame* frame);

/** Returns a copy of the latest processed image, or NULL if no frame is available. */
UFEEL_API UFeelFrame* ufeel_get_frame(UFeelProcessor* processor);

/** Releases a frame returned by ufeel_get_frame(). Accepts NULL. */
UFEEL_API void ufeel_free_frame(UFeelFrame* frame);



/** Processes the next input frame for emotions. Returns 1 when processing succeeds, otherwise 0. */
UFEEL_API int32_t ufeel_update_emotions(UFeelProcessor* processor);

/** Processes the next input frame for emotions. Returns 1 when processing succeeds, otherwise 0. */
UFEEL_API int32_t ufeel_update_emotions_from_frame(UFeelProcessor* processor, const UFeelFrame* frame);

/** Returns the current emotion scores. Set *count to zero if no scores are available. */
UFEEL_API int32_t ufeel_get_emotions_result(UFeelProcessor* processor, UFeelEmotionsResult* result);

/** Processes the input frame for emotion detection and passes the result to the handler. The result is only valid during the callback and is automatically freed afterward. */
UFEEL_API void ufeel_get_emotions_result_from_frame(UFeelProcessor* processor, UFeelFrame* frame, void (*handler)(UFeelEmotionsResult* emotions));

/** Create emotion results. */
UFEEL_API UFeelEmotionsResult* ufeel_create_emotions_result();

/** Releases emotion results returned by ufeel_get_emotions_result(). Accepts NULL. */
UFEEL_API void ufeel_free_emotions_result(UFeelEmotionsResult* emotions);



/** Processes the next input frame for eye_tracking. Returns 1 when processing succeeds, otherwise 0.*/
UFEEL_API int32_t ufeel_update_eye_tracking(UFeelProcessor* processor);

/** Processes the next input frame for eye_tracking. Returns 1 when processing succeeds, otherwise 0.*/
UFEEL_API int32_t ufeel_update_eye_tracking_from_frame(UFeelProcessor* processor, const UFeelFrame* frame);

/** Begins eye tracking calibration. */
UFEEL_API int32_t ufeel_begin_eye_tracking_calibration(UFeelProcessor* processor);

/** Sets the screen coordinate currently being used as the eye tracking calibration target. */
UFEEL_API int32_t ufeel_set_eye_tracking_calibration_target(UFeelProcessor* processor, float screenX, float screenY);

/** Fits and activates the eye tracking calibration model. */
UFEEL_API int32_t ufeel_finish_eye_tracking_calibration(UFeelProcessor* processor);

/** Aborts eye tracking calibration without activating the unfinished model. */
UFEEL_API int32_t ufeel_cancel_eye_tracking_calibration(UFeelProcessor* processor);

/** Retrieves the latest eye_tracking estimate and its validity status. */
UFEEL_API int32_t ufeel_get_eye_tracking_result(UFeelProcessor* processor, UFeelEyeTrackingResult* result);

/** Processes the input frame for eye tracking detection and passes the result to the handler. The result is only valid during the callback and is automatically freed afterward. */
UFEEL_API void ufeel_get_eye_tracking_result_from_frame(UFeelProcessor* processor, UFeelFrame* frame, void (*handler)(UFeelEyeTrackingResult* eye_tracking));


/** Retrieves progress for the current eye tracking calibration session. */
UFEEL_API int32_t ufeel_get_eye_tracking_calibration_progress(UFeelProcessor* processor, UFeelEyeTrackingCalibrationProgress* progress);

/** Retrieves diagnostic measurements from eye tracking calibration. */
UFEEL_API int32_t ufeel_get_eye_tracking_calibration_diagnostics(UFeelProcessor* processor, UFeelEyeTrackingCalibrationDiagnostics* diagnostics);

/** Saves the active eye tracking calibration profile to a UTF-8 file path. */
UFEEL_API int32_t ufeel_save_eye_tracking_calibration_profile(UFeelProcessor* processor, const char* utf8Path);

/** Loads an eye tracking calibration profile from a UTF-8 file path. */
UFEEL_API int32_t ufeel_load_eye_tracking_calibration_profile(UFeelProcessor* processor, const char* utf8Path);

/** Create eye tracking results. */
UFEEL_API UFeelEyeTrackingResult* ufeel_create_eye_tracking_result();

/** Releases eye_ racking results returned by ufeel_get_eye_tracking_result(). Accepts NULL. */
UFEEL_API void ufeel_free_eye_tracking_result(UFeelEyeTrackingResult* eye_tracking);




/** Processes the next input frame for speech. Returns 1 when processing succeeds, otherwise 0. */
UFEEL_API int32_t ufeel_update_speech_to_text(UFeelProcessor* processor);

/** Enables or disables speech-to-text processing. */
UFEEL_API int32_t ufeel_set_speech_to_text_enabled(UFeelProcessor* processor, int32_t enabled);

/** Returns the latest speech-to-text result as a library-allocated UTF-8 string. */
UFEEL_API int32_t ufeel_get_speech_to_text_result(UFeelProcessor* processor, UFeelSpeechResult* result);

/** Create speech results. */
UFEEL_API UFeelSpeechResult* ufeel_create_speech_to_text_result();

/** Releases speech result returned by ufeel_get_speech_to_text(). Accepts NULL. */
UFEEL_API void ufeel_free_speech_to_text_result(UFeelSpeechResult *speech);



/** Processes the next input frame for heartrate. Returns 1 when processing succeeds, otherwise 0. */
UFEEL_API int32_t ufeel_update_heart_rate(UFeelProcessor* processor);

/** Fetches and returns the current heartrate and its validity status. */
UFEEL_API int32_t ufeel_get_heart_rate_result(UFeelProcessor* processor, UFeelHeartRateResult* result);

/** Create heartrate results. */
UFEEL_API UFeelHeartRateResult* ufeel_create_heart_rate_result();

/** Releases heartrate result returned by ufeel_get_heartrate(). Accepts NULL. */
UFEEL_API void ufeel_free_heart_rate_result(UFeelHeartRateResult *heartrate);

#ifdef __cplusplus
}
#endif

#endif

// NOLINTEND
