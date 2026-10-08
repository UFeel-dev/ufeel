#ifndef SPEECHTOTEXT_AUDIOCAPTURE_HPP
#define SPEECHTOTEXT_AUDIOCAPTURE_HPP

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <portaudio.h>
#include <vector>

class AudioCapture
{
    public:

        static constexpr double sampleRate = 16000.0;
        static constexpr std::uint64_t framesPerBuffer = 1600;
        static constexpr std::size_t bufferCount = 8;
        static constexpr std::size_t channelCount = 2;

        AudioCapture();
        ~AudioCapture();

        AudioCapture(const AudioCapture&) = delete;
        auto operator=(const AudioCapture&) -> AudioCapture& = delete;

        AudioCapture(AudioCapture&&) = delete;
        auto operator=(AudioCapture&&) -> AudioCapture& = delete;

        auto start() -> bool;
        void stop();

        auto read(std::vector<int16_t>& buffer) -> bool;

    private:

        using AudioBuffer =
            std::array<int16_t, framesPerBuffer>;

        static_assert(
            std::atomic<std::uint64_t>::is_always_lock_free
        );

        PaStream* stream_ = nullptr;
        bool initialized_ = false;

        std::atomic<bool> running_{false};

        std::array<AudioBuffer, bufferCount> buffers_{};
        std::array<int16_t, framesPerBuffer> pendingBuffer_{};

        std::atomic<std::uint64_t> writeIndex_{0};
        std::atomic<std::uint64_t> readIndex_{0};
        std::atomic<std::uint64_t> wakeCounter_{0};

        std::size_t pendingSize_ = 0;

        auto initialize() -> bool;
        auto openStream() -> bool;
        [[nodiscard]] static auto findInputDevice() -> PaDeviceIndex;

        void closeStream();
        void terminate();

        auto push(
            const int16_t* samples,
            std::size_t frameCount
        ) -> int;

        auto pop(
            std::vector<int16_t>& buffer
        ) -> bool;

        static auto callback(
            const void* input,
            void* output,
            unsigned long frameCount,
            const PaStreamCallbackTimeInfo* timeInfo,
            PaStreamCallbackFlags statusFlags,
            void* userData
        ) -> int;
};

#endif // SPEECHTOTEXT_AUDIOCAPTURE_HPP
