#ifndef SPEECHTOTEXT_SPEECHTOTEXTDETECTOR_HPP
#define SPEECHTOTEXT_SPEECHTOTEXTDETECTOR_HPP

#include "SpeechToText/AudioSource.hpp"
#include "SpeechToText/SpeechRecognizer.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class SpeechToTextDetector
{
public:

    using Clock = std::chrono::steady_clock;

    static constexpr std::chrono::seconds kTextLifetime{3};

    explicit SpeechToTextDetector(const std::string& modelPath);
    SpeechToTextDetector(const std::string& modelPath, std::unique_ptr<AudioSource> audioSource);

    ~SpeechToTextDetector();

    SpeechToTextDetector(const SpeechToTextDetector&) = delete;
    auto operator=(const SpeechToTextDetector&) -> SpeechToTextDetector& = delete;

    SpeechToTextDetector(SpeechToTextDetector&&) = delete;
    auto operator=(SpeechToTextDetector&&) -> SpeechToTextDetector& = delete;

    void toggle(bool state);

    [[nodiscard]]
    auto process(Clock::time_point now = Clock::now()) const -> std::string;

private:

    std::unique_ptr<AudioSource> audioSource_;
    SpeechRecognizer recognizer_;

    std::thread worker_;
    std::atomic<bool> running_{false};

    mutable std::mutex textMutex_;
    std::string currentText_;
    Clock::time_point textUpdatedAt_;

    void start();
    void stop();
    void run();

    void processAudio(const std::vector<int16_t>& buffer);

    void setText(std::string text);

    [[nodiscard]]
    auto getText(Clock::time_point now) const -> std::string;
};

#endif // SPEECHTOTEXT_SPEECHTOTEXTDETECTOR_HPP
