/*
** EPITECH PROJECT, 2026
** ufeel
** File description:
** SpeechToTextDetector
*/

#ifndef SPEECHTOTEXTDETECTOR_HPP_
#define SPEECHTOTEXTDETECTOR_HPP_

#include "vosk_api.h"
#include <thread>
#include <atomic>
#include <iostream>
#include <string>
#include <portaudio.h>
#include <cstring>

class SpeechToTextDetector
{
    public:
        SpeechToTextDetector();
        ~SpeechToTextDetector();

        void toggle(bool state);
        std::string process() const;

    private:
        VoskModel* model = nullptr;
        VoskRecognizer* recognizer = nullptr;

        std::thread worker;
        std::atomic<bool> process_enable{false};
        std::atomic<bool> running{false};

        std::string current_text;

        static std::string extract_text(const char* json);

        static int paCallback(
            const void *input,
            void *,
            unsigned long frameCount,
            const PaStreamCallbackTimeInfo*,
            PaStreamCallbackFlags,
            void *userData);

        void start();
        void stop();
        void run();
};

#endif /* !SPEECHTOTEXTDETECTOR_HPP_ */
