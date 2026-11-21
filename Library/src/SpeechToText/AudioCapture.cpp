#include "SpeechToText/AudioCapture.hpp"

#include "SpeechToText/AudioSource.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <portaudio.h>
#include <string>
#include <vector>

namespace
{

// * 16 kHz needs no resampling, the recognizer downsamples the other rates
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

AudioCapture::AudioCapture()
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
}

auto AudioCapture::findFormat(
    PaDeviceIndex index, const PaDeviceInfo& info
) -> std::optional<CaptureFormat>
{
    if (info.maxInputChannels <= 0)
    {
        return std::nullopt;
    }

    const int preferredChannelCount =
        std::min(info.maxInputChannels, static_cast<int>(maxChannelCount));

    for (const double sampleRate : kSampleRates)
    {
        for (int channelCount = preferredChannelCount; channelCount > 0; --channelCount)
        {
            if (isFormatSupported(index, info, channelCount, sampleRate))
            {
                return CaptureFormat{
                    .channelCount = channelCount,
                    .sampleRate = sampleRate,
                    .latency = info.defaultLowInputLatency,
                };
            }
        }
    }

    return std::nullopt;
}

auto AudioCapture::usableDevices() -> std::vector<Device>
{
    const int deviceCount = Pa_GetDeviceCount();

    std::vector<Device> devices;

    for (int i = 0; i < deviceCount; ++i)
    {
        const auto index = static_cast<PaDeviceIndex>(i);

        const PaDeviceInfo* info = Pa_GetDeviceInfo(index);

        if (info == nullptr)
        {
            continue;
        }

        const auto format = findFormat(index, *info);

        if (format.has_value())
        {
            devices.push_back({.index = index, .name = info->name, .format = *format});
        }
    }

    return devices;
}

auto AudioCapture::availableDevices() const -> std::vector<Device>
{
    auto devices = usableDevices();

    // * a busy device fails the format probe, so the one being captured is kept explicitly
    if (stream_ != nullptr && openDevice_.has_value() &&
        std::ranges::find(devices, openDevice_->index, &Device::index) == devices.end())
    {
        devices.insert(
            std::ranges::upper_bound(devices, openDevice_->index, {}, &Device::index), *openDevice_
        );
    }

    return devices;
}

auto AudioCapture::defaultDevice(
    const std::vector<Device>& devices
) -> const Device*
{
    const PaDeviceIndex systemDefault = Pa_GetDefaultInputDevice();

    const auto found = std::ranges::find(devices, systemDefault, &Device::index);

    if (found != devices.end())
    {
        return &*found;
    }

    return devices.empty() ? nullptr : &devices.front();
}

auto AudioCapture::findInputDevice() const -> std::optional<Device>
{
    const auto devices = usableDevices();

    if (!selectedName_.empty())
    {
        const auto selected = std::ranges::find(devices, selectedName_, &Device::name);

        if (selected != devices.end())
        {
            return *selected;
        }

        std::cerr << "[AudioCapture] Microphone \"" << selectedName_
                  << "\" not found, using the default one\n";
    }

    const Device* device = defaultDevice(devices);

    return device == nullptr ? std::nullopt : std::optional<Device>(*device);
}

auto AudioCapture::microphones() -> std::vector<Microphone>
{
    rescan();

    const auto devices = availableDevices();
    const Device* fallback = defaultDevice(devices);

    std::vector<Microphone> result;
    result.reserve(devices.size());

    for (const auto& device : devices)
    {
        result.push_back({.name = device.name, .isDefault = &device == fallback});
    }

    return result;
}

auto AudioCapture::selectMicrophone(
    const std::string& name
) -> bool
{
    if (!name.empty())
    {
        rescan();

        const auto devices = availableDevices();

        if (std::ranges::find(devices, name, &Device::name) == devices.end())
        {
            return false;
        }
    }

    selectedName_ = name;

    return true;
}

auto AudioCapture::sampleRate() const -> double
{
    return format_.sampleRate;
}

auto AudioCapture::openStream() -> bool
{
    const auto device = findInputDevice();

    if (!device.has_value())
    {
        std::cerr << "[AudioCapture] No compatible input device found\n";

        return false;
    }

    std::clog << "[AudioCapture] Selected input: " << device->name << " ("
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
    openDevice_ = device;

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
