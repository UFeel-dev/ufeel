#ifndef SPEECHTOTEXT_SPEECHRECOGNIZER_HPP
#define SPEECHTOTEXT_SPEECHRECOGNIZER_HPP

#include "SpeechToText/AudioSource.hpp"
#include "vosk_api.h"

#include <cstddef>
#include <cstdint>
#include <string>

class SpeechRecognizer
{
public:

    explicit SpeechRecognizer(const std::string& modelPath);

    ~SpeechRecognizer();

    SpeechRecognizer(const SpeechRecognizer&) = delete;
    auto operator=(const SpeechRecognizer&) -> SpeechRecognizer& = delete;

    SpeechRecognizer(SpeechRecognizer&&) = delete;
    auto operator=(SpeechRecognizer&&) -> SpeechRecognizer& = delete;

    auto process(const int16_t* samples, std::size_t sampleCount) -> std::string;

    auto finalize() -> std::string;
    void reset();

    /** Keeps the current recognizer on failure. */
    [[nodiscard]] auto setSampleRate(double sampleRate) -> bool;

private:

    VoskModel* model_ = nullptr;
    VoskRecognizer* recognizer_ = nullptr;
    double sampleRate_ = AudioSource::defaultSampleRate;

    static auto extractText(const char* json, const char* key) -> std::string;

    static auto extractFinalText(const char* json) -> std::string;

    static auto extractPartialText(const char* json) -> std::string;
};

#endif // SPEECHTOTEXT_SPEECHRECOGNIZER_HPP
