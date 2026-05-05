/*
** EPITECH PROJECT, 2026
** ufeel
** File description:
** SpeechToTextDetector
*/

#ifndef SPEECHTOTEXTDETECTOR_HPP_
#define SPEECHTOTEXTDETECTOR_HPP_

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
        SpeechToTextDetector& operator=(const SpeechToTextDetector&) = delete;

        SpeechToTextDetector(SpeechToTextDetector&&) = delete;
        SpeechToTextDetector& operator=(SpeechToTextDetector&&) = delete;

        void toggle(bool state);
        [[nodiscard]] std::string process() const;

    private:
        VoskModel* model = nullptr;
        VoskRecognizer* recognizer = nullptr;

        std::thread worker;
        std::atomic<bool> process_enable{false};
        std::atomic<bool> running{false};

        std::string current_text;

        static std::string extract_text(const char* json);

        static int paCallback(const void *input, void *, uint64_t frameCount, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void *userData);

        void start();
        void stop();
        void run();
};

#endif /* !SPEECHTOTEXTDETECTOR_HPP_ */
