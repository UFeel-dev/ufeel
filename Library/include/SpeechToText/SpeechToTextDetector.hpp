/*
** EPITECH PROJECT, 2026
** ufeel
** File description:
** SpeechToTextDetector
*/

#ifndef SPEECHTOTEXT_SPEECHTOTEXTDETECTOR_HPP
#define SPEECHTOTEXT_SPEECHTOTEXTDETECTOR_HPP

#include "vosk_api.h"
#include <atomic>
#include <cstring>
#include <iostream>
#include <portaudio.h>
#include <string>
#include <thread>
#include <vector>

class SpeechToTextDetector
{
    public:
        SpeechToTextDetector();
        ~SpeechToTextDetector();

        SpeechToTextDetector(const SpeechToTextDetector&) = delete;
        auto operator=(const SpeechToTextDetector&) -> SpeechToTextDetector& = delete;

        SpeechToTextDetector(SpeechToTextDetector&&) = delete;
        auto operator=(SpeechToTextDetector&&) -> SpeechToTextDetector& = delete;

        void toggle(bool state);
        [[nodiscard]] auto process() const -> std::string;

    private:
        VoskModel* model = nullptr;
        VoskRecognizer* recognizer = nullptr;

        std::thread worker;
        std::atomic<bool> process_enable{false};
        std::atomic<bool> running{false};

        std::string current_text;

        static auto extract_text(const char* json) -> std::string;

        static auto paCallback(const void *input, void *, uint64_t frameCount, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void *userData) -> int;

        void start();
        void stop();
        void run();
};

#endif // SPEECHTOTEXT_SPEECHTOTEXTDETECTOR_HPP 
