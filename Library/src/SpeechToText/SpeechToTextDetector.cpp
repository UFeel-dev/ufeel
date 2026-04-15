/*
** EPITECH PROJECT, 2026
** ufeel
** File description:
** SpeechToTextDetector
*/

#include "SpeechToText/SpeechToTextDetector.hpp"
#include "vosk_api.h"
#include <cstring>
#include <iostream>
#include <portaudio.h>
#include <string>
#include <thread>

SpeechToTextDetector::SpeechToTextDetector()
{
    model = vosk_model_new("models/vosk-model-small-fr-0.22");
    recognizer = vosk_recognizer_new(model, 16000.0f);
    toggle(true);
}

SpeechToTextDetector::~SpeechToTextDetector()
{
    stop();
    vosk_recognizer_free(recognizer);
    vosk_model_free(model);
}

void SpeechToTextDetector::toggle(bool state)
{
    process_enable = state;
    if (state) start();
    else stop();
}

std::string SpeechToTextDetector::process() const
{
    return current_text;
}

std::string SpeechToTextDetector::extract_text(const char* json)
{
    // looks for: "text":"...."
    const char* key = "\"text\"";
    const char* p = strstr(json, key);
    if (!p) return "";

    p = strchr(p, ':');
    if (!p) return "";

    p = strchr(p, '"');
    if (!p) return "";
    p++;

    const char* end = strchr(p, '"');
    if (!end) return "";

    return std::string(p, end - p);
}

int SpeechToTextDetector::paCallback(
    const void *input,
    void *,
    unsigned long frameCount,
    const PaStreamCallbackTimeInfo*,
    PaStreamCallbackFlags,
    void *userData)
{
    auto* self = static_cast<SpeechToTextDetector*>(userData);

    if (!self->process_enable)
        return paContinue;

    const int16_t* data = (const int16_t*)input;
    int len = frameCount * sizeof(int16_t);

    if (vosk_recognizer_accept_waveform(self->recognizer,
                                        (const char*)data, len))
    {
        const char* res = vosk_recognizer_result(self->recognizer);

        std::string text = extract_text(res);

        if (!text.empty()) {
            std::cout << "Text: " << text << std::endl;
            self->current_text = text;
        }
    }

    return paContinue;
}

void SpeechToTextDetector::start()
{
    if (running) return;
    running = true;
    worker = std::thread(&SpeechToTextDetector::run, this);
}

void SpeechToTextDetector::stop()
{
    running = false;
    if (worker.joinable())
        worker.join();
}

void SpeechToTextDetector::run()
{
    Pa_Initialize();

    PaStream *stream;

    Pa_OpenDefaultStream(
        &stream,
        1,
        0,
        paInt16,
        16000,
        8000,
        paCallback,
        this
    );

    Pa_StartStream(stream);

    while (running)
        Pa_Sleep(10);

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
}
