#include "SpeechToText/AudioCapture.hpp"

#include "SpeechToText/AudioSource.hpp"
#include "SpeechToText/PulseSources.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <optional>
#include <portaudio.h>
#include <string>
#include <utility>
#include <vector>

namespace
{

constexpr std::array kSampleRates{
    AudioSource::defaultSampleRate, 48000.0, 44100.0, 32000.0, 22050.0};

auto isFormatSupported(
    PaDeviceIndex device, const PaDeviceInfo& info, int channelCount, double sampleRate
) -> bool
{
    PaStreamParameters parameters{};
    parameters.device = device;
    parameters.channelCount = channelCount;
    parameters.sampleFormat = paInt16;
    parameters.suggestedLatency = info.defaultLowInputLatency;

    return Pa_IsFormatSupported(&parameters, nullptr, sampleRate) == paFormatIsSupported;
}

} // namespace

AudioCapture::AudioCapture(
    SourceReader readSources, ChangeWatcher watchChanges
)
    : readSources_(std::move(readSources))
    , watchChanges_(std::move(watchChanges))
{
    initialize();
}

AudioCapture::~AudioCapture()
{
    stop();
    terminate();
}

auto AudioCapture::initialize() -> bool
{
    if (initialized_)
    {
        return true;
    }

    const PaError error = Pa_Initialize();

    if (error != paNoError)
    {
        std::cerr << "[AudioCapture] Pa_Initialize failed: " << Pa_GetErrorText(error) << '\n';

        return false;
    }

    initialized_ = true;

    return true;
}

void AudioCapture::rescan()
{
    if (stream_ != nullptr)
    {
        return;
    }

    terminate();
    initialize();
    devices_ = scanDevices();

    if (devices_.fromSoundServer && !changed_ && watchChanges_)
    {
        changed_ = watchChanges_();
    }
}

auto AudioCapture::isListed(
    const std::string& id
) const -> bool
{
    return std::ranges::any_of(devices_.listed, [&id](const Device& device) -> bool {
        return device.microphone.id == id;
    });
}

auto AudioCapture::findFormat(
    PaDeviceIndex index
) -> std::optional<CaptureFormat>
{
    const PaDeviceInfo* info = Pa_GetDeviceInfo(index);

    if (info == nullptr || info->maxInputChannels <= 0)
    {
        return std::nullopt;
    }

    const int preferredChannelCount =
        std::min(info->maxInputChannels, static_cast<int>(maxChannelCount));

    for (const double sampleRate : kSampleRates)
    {
        for (int channelCount = preferredChannelCount; channelCount > 0; --channelCount)
        {
            if (isFormatSupported(index, *info, channelCount, sampleRate))
            {
                return CaptureFormat{
                    .channelCount = channelCount,
                    .sampleRate = sampleRate,
                    .latency = info->defaultLowInputLatency,
                };
            }
        }
    }

    return std::nullopt;
}

auto AudioCapture::scanDevices() const -> Devices
{
    if (!initialized_)
    {
        return {};
    }

    const PaHostApiIndex pulse = Pa_HostApiTypeIdToHostApiIndex(paPulseAudio);

    if (pulse >= 0 && readSources_)
    {
        if (const auto sources = readSources_(); sources.has_value())
        {
            return pulseDevices(pulse, *sources);
        }
    }

    const PaHostApiIndex alsa = Pa_HostApiTypeIdToHostApiIndex(paALSA);

    return hostApiDevices(alsa >= 0 ? alsa : Pa_GetDefaultHostApi());
}

auto AudioCapture::pulseDevices(
    PaHostApiIndex hostApi, const PulseSources& sources
) -> Devices
{
    const PaHostApiInfo* api = Pa_GetHostApiInfo(hostApi);

    if (api == nullptr)
    {
        return {};
    }

    const auto defaultSource =
        std::ranges::find(sources.sources, sources.defaultSource, &PulseSource::name);

    Devices devices{.fromSoundServer = true};

    for (int i = 0; i < api->deviceCount; ++i)
    {
        const PaDeviceIndex index = Pa_HostApiDeviceIndexToDeviceIndex(hostApi, i);
        const PaDeviceInfo* info = Pa_GetDeviceInfo(index);

        if (info == nullptr)
        {
            continue;
        }

        const auto source =
            std::ranges::find(sources.sources, std::string(info->name), &PulseSource::name);
        const bool isDefault = index == api->defaultInputDevice;

        if (!isDefault && (source == sources.sources.end() || source->isMonitor))
        {
            continue;
        }

        const auto format = findFormat(index);

        if (!format.has_value())
        {
            continue;
        }

        if (isDefault)
        {
            devices.fallback = Device{
                .index = index,
                .microphone =
                    {
                        .name = defaultSource != sources.sources.end() ? defaultSource->description
                                                                       : info->name,
                    },
                .format = *format,
            };
            continue;
        }

        devices.listed.push_back({
            .index = index,
            .microphone =
                {
                    .id = source->name,
                    .name = source->description,
                    .isDefault = source->name == sources.defaultSource,
                },
            .format = *format,
        });
    }

    if (!devices.fallback.has_value() && !devices.listed.empty())
    {
        const auto marked = std::ranges::find_if(devices.listed, [](const Device& device) -> bool {
            return device.microphone.isDefault;
        });
        devices.fallback = marked != devices.listed.end() ? *marked : devices.listed.front();
    }

    return devices;
}

auto AudioCapture::hostApiDevices(
    PaHostApiIndex hostApi
) -> Devices
{
    const PaHostApiInfo* api = Pa_GetHostApiInfo(hostApi);

    if (api == nullptr)
    {
        return {};
    }

    Devices devices;

    for (int i = 0; i < api->deviceCount; ++i)
    {
        const PaDeviceIndex index = Pa_HostApiDeviceIndexToDeviceIndex(hostApi, i);
        const PaDeviceInfo* info = Pa_GetDeviceInfo(index);
        const auto format = findFormat(index);

        if (info != nullptr && format.has_value())
        {
            devices.listed.push_back(
                {.index = index,
                 .microphone = {.id = info->name, .name = info->name},
                 .format = *format}
            );
        }
    }

    if (devices.listed.empty())
    {
        return devices;
    }

    auto fallback = std::ranges::find(devices.listed, api->defaultInputDevice, &Device::index);

    if (fallback == devices.listed.end())
    {
        fallback = devices.listed.begin();
    }

    fallback->microphone.isDefault = true;
    devices.fallback = *fallback;

    return devices;
}

auto AudioCapture::findInputDevice() const -> std::optional<Device>
{
    if (!selectedId_.empty())
    {
        const auto selected =
            std::ranges::find_if(devices_.listed, [this](const Device& device) -> bool {
                return device.microphone.id == selectedId_;
            });

        if (selected != devices_.listed.end())
        {
            return *selected;
        }

        std::cerr << "[AudioCapture] Microphone \"" << selectedId_
                  << "\" not found, using the default one\n";
    }

    return devices_.fallback;
}

auto AudioCapture::microphones() -> std::vector<Microphone>
{
    rescan();

    std::vector<Microphone> result;
    result.reserve(devices_.listed.size());

    for (const auto& device : devices_.listed)
    {
        result.push_back(device.microphone);
    }

    return result;
}

auto AudioCapture::microphonesChanged() -> bool
{
    return changed_ && changed_();
}

auto AudioCapture::selectMicrophone(
    const std::string& id
) -> bool
{
    if (!id.empty() && !isListed(id))
    {
        rescan();

        if (!isListed(id))
        {
            return false;
        }
    }

    selectedId_ = id;

    return true;
}

auto AudioCapture::sampleRate() const -> double
{
    return format_.sampleRate;
}

auto AudioCapture::openStream() -> bool
{
    rescan();

    const auto device = findInputDevice();

    if (!device.has_value())
    {
        std::cerr << "[AudioCapture] No compatible input device found\n";

        return false;
    }

    std::clog << "[AudioCapture] Capturing from " << device->microphone.name << " ("
              << device->format.channelCount << " ch, " << device->format.sampleRate << " Hz)\n";

    PaStreamParameters inputParameters{};
    inputParameters.device = device->index;
    inputParameters.channelCount = device->format.channelCount;
    inputParameters.sampleFormat = paInt16;
    inputParameters.suggestedLatency = device->format.latency;

    const PaError error = Pa_OpenStream(
        &stream_,
        &inputParameters,
        nullptr,
        device->format.sampleRate,
        framesPerBuffer,
        paNoFlag,
        callback,
        this
    );

    if (error != paNoError)
    {
        std::cerr << "[AudioCapture] Pa_OpenStream failed: " << Pa_GetErrorText(error) << '\n';

        stream_ = nullptr;
        return false;
    }

    format_ = device->format;

    return true;
}

auto AudioCapture::start() -> bool
{
    if (!initialized_ && !initialize())
    {
        return false;
    }

    if (stream_ == nullptr && !openStream())
    {
        return false;
    }

    writeIndex_.store(0, std::memory_order_relaxed);

    readIndex_.store(0, std::memory_order_relaxed);

    pendingSize_ = 0;

    running_.store(true, std::memory_order_release);

    const PaError error = Pa_StartStream(stream_);

    if (error != paNoError)
    {
        running_.store(false, std::memory_order_release);

        wakeCounter_.fetch_add(1, std::memory_order_relaxed);

        wakeCounter_.notify_all();

        std::cerr << "[AudioCapture] Pa_StartStream failed: " << Pa_GetErrorText(error) << '\n';

        closeStream();

        return false;
    }

    return true;
}

void AudioCapture::stop()
{
    if (stream_ == nullptr)
    {
        return;
    }

    running_.store(false, std::memory_order_release);

    wakeCounter_.fetch_add(1, std::memory_order_relaxed);

    wakeCounter_.notify_all();

    if (Pa_IsStreamActive(stream_) == 1)
    {
        const PaError error = Pa_StopStream(stream_);

        if (error != paNoError)
        {
            std::cerr << "[AudioCapture] Pa_StopStream failed: " << Pa_GetErrorText(error) << '\n';
        }
    }

    closeStream();
}

void AudioCapture::closeStream()
{
    if (stream_ == nullptr)
    {
        return;
    }

    Pa_CloseStream(stream_);
    stream_ = nullptr;
}

void AudioCapture::terminate()
{
    if (!initialized_)
    {
        return;
    }

    Pa_Terminate();
    initialized_ = false;
}

auto AudioCapture::push(
    const int16_t* samples, std::size_t frameCount
) -> int
{
    if (!running_.load(std::memory_order_acquire))
    {
        return paComplete;
    }

    const auto channelCount = static_cast<std::size_t>(format_.channelCount);

    for (std::size_t frame = 0; frame < frameCount; ++frame)
    {
        const std::size_t index = frame * channelCount;

        int32_t sum = 0;

        for (std::size_t channel = 0; channel < channelCount; ++channel)
        {
            sum += samples[index + channel];
        }

        pendingBuffer_.at(pendingSize_) =
            static_cast<int16_t>(sum / static_cast<int32_t>(channelCount));

        ++pendingSize_;

        if (pendingSize_ < framesPerBuffer)
        {
            continue;
        }

        const std::uint64_t writeIndex = writeIndex_.load(std::memory_order_relaxed);

        const std::uint64_t readIndex = readIndex_.load(std::memory_order_acquire);

        if (writeIndex - readIndex >= bufferCount)
        {
            pendingSize_ = 0;
            continue;
        }

        buffers_.at(writeIndex % bufferCount) = pendingBuffer_;

        writeIndex_.store(writeIndex + 1, std::memory_order_release);

        wakeCounter_.fetch_add(1, std::memory_order_relaxed);

        wakeCounter_.notify_one();

        pendingSize_ = 0;
    }

    return paContinue;
}

auto AudioCapture::pop(
    std::vector<int16_t>& buffer
) -> bool
{
    for (;;)
    {
        const std::uint64_t wake = wakeCounter_.load(std::memory_order_relaxed);

        const std::uint64_t readIndex = readIndex_.load(std::memory_order_relaxed);

        const std::uint64_t writeIndex = writeIndex_.load(std::memory_order_acquire);

        if (readIndex != writeIndex)
        {
            const AudioBuffer& source = buffers_.at(readIndex % bufferCount);

            buffer.assign(source.begin(), source.end());

            readIndex_.store(readIndex + 1, std::memory_order_release);

            return true;
        }

        if (!running_.load(std::memory_order_acquire))
        {
            return false;
        }

        wakeCounter_.wait(wake, std::memory_order_relaxed);
    }
}

auto AudioCapture::read(
    std::vector<int16_t>& buffer
) -> bool
{
    return pop(buffer);
}

auto AudioCapture::callback(
    const void* input,
    void* /*output*/,
    unsigned long frameCount,
    const PaStreamCallbackTimeInfo* /*timeInfo*/,
    PaStreamCallbackFlags /*statusFlags*/,
    void* userData
) -> int
{
    auto* capture = static_cast<AudioCapture*>(userData);

    if (input == nullptr)
    {
        return paContinue;
    }

    return capture->push(static_cast<const int16_t*>(input), frameCount);
}
