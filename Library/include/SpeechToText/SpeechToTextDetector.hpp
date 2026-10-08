#ifndef SPEECHTOTEXT_SPEECHTOTEXTDETECTOR_HPP
#define SPEECHTOTEXT_SPEECHTOTEXTDETECTOR_HPP

#include "SpeechToText/AudioCapture.hpp"
#include "SpeechToText/SpeechRecognizer.hpp"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class SpeechToTextDetector
{
    public:

        explicit SpeechToTextDetector(
            const std::string& modelPath
        );

        ~SpeechToTextDetector();

        SpeechToTextDetector(const SpeechToTextDetector&) = delete;
        auto operator=(const SpeechToTextDetector&) -> SpeechToTextDetector& = delete;

        SpeechToTextDetector(SpeechToTextDetector&&) = delete;
        auto operator=(SpeechToTextDetector&&) -> SpeechToTextDetector& = delete;

        void toggle(bool state);

        [[nodiscard]]
        auto process() const -> std::string;

    private:

        AudioCapture audioCapture_;
        SpeechRecognizer recognizer_;

        std::thread worker_;
        std::atomic<bool> running_{false};

        mutable std::mutex textMutex_;
        std::string currentText_;

        void start();
        void stop();
        void run();

        void processAudio(
            const std::vector<int16_t>& buffer
        );

        void setText(std::string text);

        [[nodiscard]]
        auto getText() const -> std::string;
};

#endif // SPEECHTOTEXT_SPEECHTOTEXTDETECTOR_HPP
