#include "SpeechToText/AudioCapture.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <portaudio.h>
#include <string>
#include <vector>

AudioCapture::AudioCapture()
{
    initialize();
}

AudioCapture::~AudioCapture()
{
    stop();
    closeStream();
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

static auto getWorkingSampleRate(PaDeviceIndex device, int channelCount, PaTime defaultLowInputLatency) -> double
{
    const double standardSampleRates[] = { 48000.0, 44100.0, 32000.0, 24000.0, 16000.0, 11025.0, 8000.0 };

    PaStreamParameters parameters{};

    parameters.device = device;
    parameters.channelCount = channelCount;
    parameters.sampleFormat = paInt16;
    parameters.suggestedLatency = defaultLowInputLatency;
    parameters.hostApiSpecificStreamInfo = nullptr;

    for (double testRate : standardSampleRates)
    {
        if (Pa_IsFormatSupported(&parameters, nullptr, testRate) == paFormatIsSupported)
        {
            return testRate;
        }
    }

    return 0.0;
}

void AudioCapture::getMicList(std::vector<int>& micList)
{
    const int deviceCount = Pa_GetDeviceCount();

    if (deviceCount < 0)
    {
        return;
    }

    for (int i = 0; i < deviceCount; ++i)
    {
        const auto device = static_cast<PaDeviceIndex>(i);

        const PaDeviceInfo* info = Pa_GetDeviceInfo(device);
        
        if (info == nullptr)
        {
            continue;
        }

        if (info->maxInputChannels <= 0)
        {
            continue;
        }

        double sampleRate = getWorkingSampleRate(device, info->maxInputChannels, info->defaultLowInputLatency);

        if (sampleRate != 0.0)
        {
            std::clog << "--- ACCEPTED DEVICE " << device << ": name = " << info->name << " ; channelCount = " << info->maxInputChannels << " ; working SampleRate = " << sampleRate << " ---\n";
            micList.push_back(device);
        }
    }
}

void AudioCapture::getMicName(PaDeviceIndex device, std::string &output)
{
    const PaDeviceInfo* info = Pa_GetDeviceInfo(device);
        
    if (info == nullptr)
    {
        output = "Mic name not found";
        return;
    }

    output = info->name;
}

auto AudioCapture::getMicSampleRate(PaDeviceIndex device) -> double
{
    const PaDeviceInfo* info = Pa_GetDeviceInfo(device);
    
    if (info == nullptr)
    {
        return 0;
    }

    return getWorkingSampleRate(device, info->maxInputChannels, info->defaultLowInputLatency);
}

auto AudioCapture::openStream(PaDeviceIndex device) -> bool
{
    if (device == paNoDevice)
    {
        std::cerr << "[AudioCapture] No compatible input device found (openStream)\n";

        return false;
    }

    const PaDeviceInfo* info = Pa_GetDeviceInfo(device);

    if (info == nullptr)
    {
        return false;
    }

    std::clog << "[AudioCapture] Selected input: " << info->name << '\n';

    PaStreamParameters inputParameters{};
    inputParameters.device = device;
    inputParameters.channelCount = info->maxInputChannels;
    inputParameters.sampleFormat = paInt16;
    inputParameters.suggestedLatency = info->defaultLowInputLatency;

    channelCount_ = info->maxInputChannels;

    const PaError error = Pa_OpenStream(
        &stream_, &inputParameters, nullptr, info->defaultSampleRate, framesPerBuffer, paNoFlag, callback, this
    ); // TODO: replace sampleRate by calculated one (get with struct <device index, samplerate> ?)

    if (error != paNoError)
    {
        std::cerr << "[AudioCapture] Pa_OpenStream failed: " << Pa_GetErrorText(error) << '\n';

        stream_ = nullptr;
        return false;
    }

    return true;
}

auto AudioCapture::start() -> bool
{
    if (!initialized_ && !initialize())
    {
        return false;
    }

    if (stream_ == nullptr && !openStream(paNoDevice))
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

    for (std::size_t frame = 0; frame < frameCount; ++frame)
    {
        const std::size_t index = frame * channelCount_;

        const int32_t left = samples[index];

        const int32_t right = samples[index + 1];

        pendingBuffer_.at(pendingSize_) = static_cast<int16_t>((left + right) / 2);

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
