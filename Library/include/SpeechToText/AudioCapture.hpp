#ifndef SPEECHTOTEXT_AUDIOCAPTURE_HPP
#define SPEECHTOTEXT_AUDIOCAPTURE_HPP

#include "SpeechToText/AudioSource.hpp"
#include "portaudio.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

class AudioCapture final : public AudioSource
{
public:

    static constexpr std::uint64_t framesPerBuffer = 1600;
    static constexpr std::size_t bufferCount = 32;
    static constexpr std::size_t maxChannelCount = 2;

    AudioCapture();
    ~AudioCapture() override;

    AudioCapture(const AudioCapture&) = delete;
    auto operator=(const AudioCapture&) -> AudioCapture& = delete;

    AudioCapture(AudioCapture&&) = delete;
    auto operator=(AudioCapture&&) -> AudioCapture& = delete;

    auto start() -> bool override;
    void stop() override;

    auto read(std::vector<int16_t>& buffer) -> bool override;

    [[nodiscard]] auto sampleRate() const -> double override;

    /** Rescans the devices while no stream is open. */
    [[nodiscard]] auto microphones() -> std::vector<Microphone> override;
    auto selectMicrophone(const std::string& name) -> bool override;

private:

    using AudioBuffer = std::array<int16_t, framesPerBuffer>;

    struct CaptureFormat
    {
        int channelCount = 0;
        double sampleRate = AudioSource::defaultSampleRate;
        PaTime latency = 0.0;
    };

    struct Device
    {
        PaDeviceIndex index = paNoDevice;
        std::string name;
        CaptureFormat format;
    };

    static_assert(std::atomic<std::uint64_t>::is_always_lock_free);

    PaStream* stream_ = nullptr;
    bool initialized_ = false;
    std::string selectedName_;
    std::optional<Device> openDevice_;
    CaptureFormat format_;

    std::atomic<bool> running_{false};

    std::array<AudioBuffer, bufferCount> buffers_{};
    std::array<int16_t, framesPerBuffer> pendingBuffer_{};

    std::atomic<std::uint64_t> writeIndex_{0};
    std::atomic<std::uint64_t> readIndex_{0};
    std::atomic<std::uint64_t> wakeCounter_{0};

    std::size_t pendingSize_ = 0;

    auto initialize() -> bool;
    auto openStream() -> bool;
    void rescan();

    [[nodiscard]] static auto findFormat(PaDeviceIndex index, const PaDeviceInfo& info)
        -> std::optional<CaptureFormat>;
    [[nodiscard]] static auto usableDevices() -> std::vector<Device>;
    [[nodiscard]] auto availableDevices() const -> std::vector<Device>;
    [[nodiscard]] static auto defaultDevice(const std::vector<Device>& devices) -> const Device*;
    [[nodiscard]] auto findInputDevice() const -> std::optional<Device>;

    void closeStream();
    void terminate();

    auto push(const int16_t* samples, std::size_t frameCount) -> int;

    auto pop(std::vector<int16_t>& buffer) -> bool;

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
