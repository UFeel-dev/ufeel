#ifndef SPEECHTOTEXT_AUDIOCAPTURE_HPP
#define SPEECHTOTEXT_AUDIOCAPTURE_HPP

#include "SpeechToText/AudioSource.hpp"
#include "SpeechToText/PulseSources.hpp"
#include "portaudio.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

class AudioCapture final : public AudioSource
{
public:

    using SourceReader = std::function<std::optional<PulseSources>()>;
    using ChangeWatcher = std::function<std::function<bool()>()>;

    static constexpr std::uint64_t framesPerBuffer = 1600;
    static constexpr std::size_t bufferCount = 32;
    static constexpr std::size_t maxChannelCount = 2;

    explicit AudioCapture(
        SourceReader readSources = readPulseSources, ChangeWatcher watchChanges = watchPulseSources
    );
    ~AudioCapture() override;

    AudioCapture(const AudioCapture&) = delete;
    auto operator=(const AudioCapture&) -> AudioCapture& = delete;

    AudioCapture(AudioCapture&&) = delete;
    auto operator=(AudioCapture&&) -> AudioCapture& = delete;

    auto start() -> bool override;
    void stop() override;

    auto read(std::vector<int16_t>& buffer) -> bool override;

    [[nodiscard]] auto sampleRate() const -> double override;

    /** Rescans the devices only while no stream is open. */
    [[nodiscard]] auto microphones() -> std::vector<Microphone> override;
    [[nodiscard]] auto microphonesChanged() -> bool override;
    auto selectMicrophone(const std::string& id) -> bool override;

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
        Microphone microphone;
        CaptureFormat format;
    };

    struct Devices
    {
        std::vector<Device> listed;
        std::optional<Device> fallback;
        bool fromSoundServer = false;
    };

    static_assert(std::atomic<std::uint64_t>::is_always_lock_free);

    SourceReader readSources_;
    ChangeWatcher watchChanges_;
    std::function<bool()> changed_;
    Devices devices_;
    std::string selectedId_;

    PaStream* stream_ = nullptr;
    bool initialized_ = false;
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
    [[nodiscard]] auto isListed(const std::string& id) const -> bool;

    [[nodiscard]] auto scanDevices() const -> Devices;
    [[nodiscard]] static auto findFormat(PaDeviceIndex index) -> std::optional<CaptureFormat>;
    [[nodiscard]] static auto pulseDevices(PaHostApiIndex hostApi, const PulseSources& sources)
        -> Devices;
    [[nodiscard]] static auto hostApiDevices(PaHostApiIndex hostApi) -> Devices;
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
