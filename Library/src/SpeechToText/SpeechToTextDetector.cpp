/*
** EPITECH PROJECT, 2026
** ufeel
** File description:
** SpeechToTextDetector
*/

#include "SpeechToText/SpeechToTextDetector.hpp"

SpeechToTextDetector::SpeechToTextDetector()
{
    model = vosk_model_new("models/vosk-model-small-fr-0.22");
    recognizer = vosk_recognizer_new(model, 16000.0F);
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
    if (state) {
        start();
    } else {
        stop();
    }
}

std::string SpeechToTextDetector::process() const
{
    return current_text;
}

std::string SpeechToTextDetector::extract_text(const char *json)
{
    static const std::vector<char> parse = {
        ':',
        '"'
    };
    static const char *key = "\"text\"";
    const char *p = strstr(json, key);
    if (p == nullptr) {
        return "";
    }

    for (auto parseChar: parse) {
        p = strchr(p, parseChar);
        if (p == nullptr) {
            return "";
        }
    }
    p++;

    const char* end = strchr(p, '"');
    if (end == nullptr) {
        return "";
    }

    return {p, static_cast<std::size_t>(end - p)};
}

int SpeechToTextDetector::paCallback(const void *input, void */*_*/, uint64_t frameCount, const PaStreamCallbackTimeInfo */*_*/, PaStreamCallbackFlags /*_*/, void *userData)
{
    auto* self = static_cast<SpeechToTextDetector*>(userData);

    if (!self->process_enable) {
        return paContinue;
    }

    const auto* data = static_cast<const int16_t*>(input);
    int len = static_cast<int>(frameCount * sizeof(int16_t));

    if (vosk_recognizer_accept_waveform(self->recognizer, reinterpret_cast<const char*>(data), len) != 0) {
        const char* res = vosk_recognizer_result(self->recognizer);

        std::string text = extract_text(res);

        if (!text.empty()) {
            self->current_text = text;
        }
    }

    return paContinue;
}

void SpeechToTextDetector::start()
{
    if (running) {
        return;
    }
    running = true;
    worker = std::thread(&SpeechToTextDetector::run, this);
}

void SpeechToTextDetector::stop()
{
    running = false;
    if (worker.joinable()) {
        worker.join();
    }
}

void SpeechToTextDetector::run()
{
    Pa_Initialize();

    PaStream *stream = nullptr;

    Pa_OpenDefaultStream(&stream, 1, 0, paInt16, 16000, 8000, paCallback, this);
    Pa_StartStream(stream);

    while (running) {
        Pa_Sleep(10);
    }

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
}
