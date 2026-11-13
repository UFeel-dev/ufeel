#ifndef LIBRARY_SPEECHTOTEXT_TESTSUPPORT_WAVAUDIOSOURCE_HPP
#define LIBRARY_SPEECHTOTEXT_TESTSUPPORT_WAVAUDIOSOURCE_HPP

#include "SpeechToText/AudioCapture.hpp"
#include "SpeechToText/AudioSource.hpp"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

inline auto readWav(
    const std::filesystem::path& path
) -> std::vector<int16_t>
{
    std::ifstream file(path, std::ios::binary);
    const std::vector<char> bytes{std::istreambuf_iterator<char>(file), {}};

    auto readUint = [&bytes](std::size_t offset, std::size_t size) -> std::uint32_t {
        std::uint32_t value = 0;
        std::memcpy(&value, bytes.data() + offset, size);
        return value;
    };
    auto tagAt = [&bytes](std::size_t offset) -> std::string { return {bytes.data() + offset, 4}; };

    if (bytes.size() < 12 || tagAt(0) != "RIFF" || tagAt(8) != "WAVE")
    {
        throw std::runtime_error("Not a WAV file: " + path.string());
    }

    bool supportedFormat = false;
    for (std::size_t offset = 12; offset + 8 <= bytes.size();)
    {
        const std::string tag = tagAt(offset);
        const std::size_t size = readUint(offset + 4, 4);
        const std::size_t body = offset + 8;
        if (body + size > bytes.size())
        {
            break;
        }

        if (tag == "fmt " && size >= 16)
        {
            supportedFormat =
                readUint(body, 2) == 1 && readUint(body + 2, 2) == 1 &&
                readUint(body + 4, 4) == static_cast<std::uint32_t>(AudioCapture::sampleRate) &&
                readUint(body + 14, 2) == 16;
        }
        else if (tag == "data" && supportedFormat)
        {
            std::vector<int16_t> samples(size / sizeof(int16_t));
            std::memcpy(samples.data(), bytes.data() + body, samples.size() * sizeof(int16_t));
            return samples;
        }

        offset = body + size + (size % 2);
    }

    throw std::runtime_error("Expected 16 kHz mono 16-bit PCM WAV: " + path.string());
}

class WavAudioSource final : public AudioSource
{
public:

    explicit WavAudioSource(
        std::vector<int16_t> samples
    )
        : samples_(std::move(samples))
    {
    }

    auto start() -> bool override
    {
        const std::scoped_lock lock(mutex_);
        position_ = 0;
        drained_ = false;
        stopped_ = false;
        return true;
    }

    void stop() override
    {
        {
            const std::scoped_lock lock(mutex_);
            stopped_ = true;
        }
        changed_.notify_all();
    }

    auto read(
        std::vector<int16_t>& buffer
    ) -> bool override
    {
        std::unique_lock lock(mutex_);
        if (position_ == samples_.size())
        {
            drained_ = true;
            changed_.notify_all();
            changed_.wait(lock, [this] -> bool { return stopped_; });
            return false;
        }

        const auto count =
            std::min<std::size_t>(AudioCapture::framesPerBuffer, samples_.size() - position_);
        const auto first = samples_.begin() + static_cast<std::ptrdiff_t>(position_);
        buffer.assign(first, first + static_cast<std::ptrdiff_t>(count));
        position_ += count;
        return true;
    }

    auto waitUntilDrained() -> bool
    {
        std::unique_lock lock(mutex_);
        return changed_.wait_for(lock, std::chrono::seconds(30), [this] -> bool {
            return drained_;
        });
    }

private:

    std::vector<int16_t> samples_;
    std::size_t position_ = 0;
    bool drained_ = false;
    bool stopped_ = false;
    std::mutex mutex_;
    std::condition_variable changed_;
};

#endif // LIBRARY_SPEECHTOTEXT_TESTSUPPORT_WAVAUDIOSOURCE_HPP
