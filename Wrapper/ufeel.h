// NOLINTBEGIN

#ifndef UFEEL_H
#define UFEEL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
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

typedef struct UFeelBoolPair
{
    const char* key;
    uint8_t value;
} UFeelBoolPair;

void* ufeel_create(const char* speech_model_path);
void ufeel_destroy(void* processor);

int32_t ufeel_update(void* processor);

UFeelFrame* ufeel_get_frame(void* processor);
void ufeel_free_frame(UFeelFrame* frame);

UFeelPair* ufeel_get_emotions(
    void* processor,
    uint32_t* size
);

void ufeel_free_emotions(
    UFeelPair* emotions,
    uint32_t size
);

void ufeel_calibrate_directions(void* processor);

UFeelBoolPair* ufeel_get_directions(
    void* processor,
    uint32_t* size
);

void ufeel_free_directions(
    UFeelBoolPair* directions,
    uint32_t size
);

void ufeel_toggle_speech(
    void* processor,
    uint8_t state
);

char* ufeel_get_speech(void* processor);
void ufeel_free_speech(char* speech);

#ifdef __cplusplus
}
#endif

#endif

// NOLINTEND
