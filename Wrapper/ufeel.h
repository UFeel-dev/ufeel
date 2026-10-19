// NOLINTBEGIN

#ifndef UFEEL_H
#define UFEEL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UFeelFrame
{
    uint8_t* data;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
} UFeelFrame;

typedef struct UFeelPair
{
    const char* key;
    float value;
} UFeelPair;

typedef struct UFeelGaze
{
    float x;
    float y;
    uint32_t directions;
    uint32_t valid;
} UFeelGaze;

typedef struct UFeelGazeEx
{
    float x;
    float y;
    float confidence;
    uint32_t directions;
    uint32_t status;
} UFeelGazeEx; // TODO: improve naming for UFeelGaze, UFeelGazeEx and UFeelGazeResult (currently
               // hard to tell what does what)

typedef struct UFeelGazeResult
{
    float recorded_x;
    float recorded_y;
    float estimated_x;
    float estimated_y;
    float confidence;
    uint32_t directions;
    uint32_t status;
} UFeelGazeResult;

typedef struct UFeelCalibrationProgress
{
    uint32_t targets_with_enough_samples;
    uint32_t target_count;
    uint32_t samples_for_current_target;
    uint32_t required_samples_per_target;
} UFeelCalibrationProgress;

/* Return values for calibration, gaze, and profile calls. */
typedef enum UFeelStatus
{
    UFEEL_STATUS_OK = 0,
    UFEEL_STATUS_NO_GAZE = 1,
    UFEEL_STATUS_INVALID_ARGUMENT = 2,
    UFEEL_STATUS_INVALID_STATE = 3,
    UFEEL_STATUS_CALIBRATION_FAILED = 4,
    UFEEL_STATUS_PROFILE_FAILED = 5
} UFeelStatus;

/* State of the latest gaze result. */
typedef enum UFeelGazeStatus
{
    UFEEL_GAZE_NOT_CALIBRATED = 0,
    UFEEL_GAZE_FACE_UNAVAILABLE = 1,
    UFEEL_GAZE_BLINKING = 2,
    UFEEL_GAZE_LOW_QUALITY = 3,
    UFEEL_GAZE_CALIBRATING = 4,
    UFEEL_GAZE_VALID = 5,
    UFEEL_GAZE_DISABLED = 6,
    UFEEL_GAZE_FRAME_UNAVAILABLE = 7
} UFeelGazeStatus;

/* Gaze direction bit flags, multiple non-center flags may be set together. */
enum UFeelGazeDirection
{
    UFEEL_GAZE_LEFT = 1U << 0U,
    UFEEL_GAZE_RIGHT = 1U << 1U,
    UFEEL_GAZE_UP = 1U << 2U,
    UFEEL_GAZE_DOWN = 1U << 3U,
    UFEEL_GAZE_CENTER = 1U << 4U
};

/* Both paths must refer to directories in the installed package. */
void* ufeel_create(const char* model_directory, const char* speech_model_path);
void ufeel_destroy(void* processor);

int32_t ufeel_update(void* processor);

UFeelFrame* ufeel_get_frame(void* processor);
void ufeel_free_frame(UFeelFrame* frame);

UFeelPair* ufeel_get_emotions(void* processor, uint32_t* size);

void ufeel_free_emotions(UFeelPair* emotions, uint32_t size);

/* Calibration target coordinates are normalized over the caller's client area. */
int32_t ufeel_begin_calibration(void* processor);
int32_t ufeel_set_calibration_target(void* processor, float x, float y);
int32_t ufeel_finish_calibration(void* processor);
int32_t ufeel_cancel_calibration(void* processor);
int32_t ufeel_get_gaze(void* processor, UFeelGaze* gaze);
/* Extended result: confidence and a UFeelGazeStatus are written even when no gaze is available. */
int32_t ufeel_get_gaze_ex(void* processor, UFeelGazeEx* gaze);
int32_t ufeel_get_gaze_result(void* processor, UFeelGazeResult* gaze);
/* Progress counts valid, non-blink samples for the current and completed targets. */
int32_t ufeel_get_calibration_progress(void* processor, UFeelCalibrationProgress* progress);
/* Profile paths are caller-owned writable/readable paths, separate from installed model assets. */
int32_t ufeel_save_calibration_profile(void* processor, const char* profile_path);
int32_t ufeel_load_calibration_profile(void* processor, const char* profile_path);

/* Calls operating on the same processor instance must be serialized by the caller. */
void ufeel_toggle_speech(void* processor, uint8_t state);

char* ufeel_get_speech(void* processor);
void ufeel_free_speech(char* speech);

#ifdef __cplusplus
}
#endif

#endif

// NOLINTEND
