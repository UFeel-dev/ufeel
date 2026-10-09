#include "SpeechToText/SpeechToTextDetector.hpp"

#include "SpeechToText/AudioCapture.hpp"

#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

SpeechToTextDetector::SpeechToTextDetector(
    const std::string& modelPath
)
    : recognizer_(modelPath)
{
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

auto SpeechToTextDetector::process() const -> std::string
{
    return getText();
}

void SpeechToTextDetector::start()
{
    if (running_.exchange(true))
    {
        return;
    }

    if (!audioCapture_.start())
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

    audioCapture_.stop();

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
        if (!audioCapture_.read(buffer))
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
    std::lock_guard lock(textMutex_);
    currentText_ = std::move(text);
}

auto SpeechToTextDetector::getText() const -> std::string
{
    std::lock_guard lock(textMutex_);
    return currentText_;
}
