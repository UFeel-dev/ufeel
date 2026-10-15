#include "SpeechToText/SpeechRecognizer.hpp"

#include "vosk_api.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

SpeechRecognizer::SpeechRecognizer(
    const std::string& modelPath
)
{
    model_ = vosk_model_new(modelPath.c_str());

    if (model_ == nullptr)
    {
        throw std::runtime_error("Failed to load Vosk model.");
    }

    recognizer_ = vosk_recognizer_new(model_, sampleRate);

    if (recognizer_ == nullptr)
    {
        vosk_model_free(model_);
        model_ = nullptr;

        throw std::runtime_error("Failed to create Vosk recognizer.");
    }
}

SpeechRecognizer::~SpeechRecognizer()
{
    if (recognizer_ != nullptr)
    {
        vosk_recognizer_free(recognizer_);
        recognizer_ = nullptr;
    }

    if (model_ != nullptr)
    {
        vosk_model_free(model_);
        model_ = nullptr;
    }
}

auto SpeechRecognizer::process(
    const int16_t* samples, std::size_t sampleCount
) -> std::string
{
    const int bytes = static_cast<int>(sampleCount * sizeof(int16_t));

    const int accepted =
        vosk_recognizer_accept_waveform(recognizer_, reinterpret_cast<const char*>(samples), bytes);

    if (accepted != 0)
    {
        return extractFinalText(vosk_recognizer_result(recognizer_));
    }

    return extractPartialText(vosk_recognizer_partial_result(recognizer_));
}

auto SpeechRecognizer::finalize() -> std::string
{
    return extractFinalText(vosk_recognizer_final_result(recognizer_));
}

void SpeechRecognizer::reset()
{
    vosk_recognizer_reset(recognizer_);
}

auto SpeechRecognizer::extractText(
    const char* json, const char* key
) -> std::string
{
    if (json == nullptr)
    {
        return {};
    }

    const std::string value(json);
    const std::string search = std::string("\"") + key + "\"";

    const std::size_t keyPosition = value.find(search);

    if (keyPosition == std::string::npos)
    {
        return {};
    }

    const std::size_t start = value.find('"', keyPosition + search.size());

    if (start == std::string::npos)
    {
        return {};
    }

    const std::size_t end = value.find('"', start + 1);

    if (end == std::string::npos)
    {
        return {};
    }

    return value.substr(start + 1, end - start - 1);
}

auto SpeechRecognizer::extractFinalText(
    const char* json
) -> std::string
{
    return extractText(json, "text");
}

auto SpeechRecognizer::extractPartialText(
    const char* json
) -> std::string
{
    return extractText(json, "partial");
}
