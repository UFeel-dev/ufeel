#include "SpeechToText/SpeechToTextDetector.hpp"

#include "SpeechToText/AudioCapture.hpp"
#include "SpeechToText/AudioSource.hpp"

#include <cstdint>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

SpeechToTextDetector::SpeechToTextDetector(
    const std::string& modelPath
)
    : SpeechToTextDetector(modelPath, std::make_unique<AudioCapture>())
{
}

SpeechToTextDetector::SpeechToTextDetector(
    const std::string& modelPath, std::unique_ptr<AudioSource> audioSource
)
    : audioSource_(std::move(audioSource))
    , recognizer_(modelPath)
{
    if (!audioSource_)
    {
        throw std::invalid_argument("speech-to-text audio source must not be null");
    }
}

SpeechToTextDetector::~SpeechToTextDetector()
{
    stop();
}

void SpeechToTextDetector::toggle(
    bool state
)
{
    if (state)
    {
        start();
        return;
    }

    stop();
}

auto SpeechToTextDetector::process(
    Clock::time_point now
) const -> std::string
{
    return getText(now);
}

void SpeechToTextDetector::start()
{
    if (running_.exchange(true))
    {
        return;
    }

    if (!audioSource_->start())
    {
        running_ = false;
        return;
    }

    worker_ = std::thread(&SpeechToTextDetector::run, this);
}

void SpeechToTextDetector::stop()
{
    if (!running_.exchange(false))
    {
        return;
    }

    audioSource_->stop();

    if (worker_.joinable())
    {
        worker_.join();
    }
}

void SpeechToTextDetector::run()
{
    std::vector<int16_t> buffer(AudioCapture::framesPerBuffer);

    while (running_)
    {
        if (!audioSource_->read(buffer))
        {
            break;
        }

        processAudio(buffer);
    }

    const std::string finalText = recognizer_.finalize();

    if (!finalText.empty())
    {
        setText(finalText);
    }
}

void SpeechToTextDetector::processAudio(
    const std::vector<int16_t>& buffer
)
{
    const std::string text = recognizer_.process(buffer.data(), buffer.size());

    if (!text.empty())
    {
        setText(text);
    }
}

void SpeechToTextDetector::setText(
    std::string text
)
{
    std::scoped_lock const lock(textMutex_);
    currentText_ = std::move(text);
    textUpdatedAt_ = Clock::now();
}

auto SpeechToTextDetector::getText(
    Clock::time_point now
) const -> std::string
{
    std::scoped_lock const lock(textMutex_);
    if (now - textUpdatedAt_ > kTextLifetime)
    {
        return {};
    }
    return currentText_;
}
