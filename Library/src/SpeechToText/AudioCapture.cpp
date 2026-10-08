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
        std::cerr
            << "[AudioCapture] Pa_Initialize failed: "
            << Pa_GetErrorText(error)
            << '\n';

        return false;
    }

    initialized_ = true;

    return true;
}

// ! this will not always find the most appropriate mic
auto AudioCapture::findInputDevice() -> PaDeviceIndex
{
    const int deviceCount =
        Pa_GetDeviceCount();

    if (deviceCount < 0)
    {
        return paNoDevice;
    }

    PaDeviceIndex bestDevice = paNoDevice;
    int bestScore = -1;

    for (int i = 0; i < deviceCount; ++i)
    {
        const auto device =
            static_cast<PaDeviceIndex>(i);

        const PaDeviceInfo* info =
            Pa_GetDeviceInfo(device);

        if (info == nullptr ||
            info->maxInputChannels < channelCount)
        {
            continue;
        }

        PaStreamParameters parameters{};
        parameters.device = device;
        parameters.channelCount = channelCount;
        parameters.sampleFormat = paInt16;
        parameters.suggestedLatency =
            info->defaultLowInputLatency;

        if (Pa_IsFormatSupported(
                &parameters,
                nullptr,
                sampleRate
            ) != paNoError)
        {
            continue;
        }

        int score = 0;

        if (info->defaultSampleRate == sampleRate)
        {
            score += 100;
        }

        if (std::string(info->name).find("(hw:") !=
            std::string::npos)
        {
            score += 50;
        }

        if (std::string(info->name).find("default") ==
            std::string::npos)
        {
            score += 10;
        }

        if (score > bestScore)
        {
            bestScore = score;
            bestDevice = device;
        }
    }

    return bestDevice;
}

auto AudioCapture::openStream() -> bool
{
    const PaDeviceIndex device =
        findInputDevice();

    if (device == paNoDevice)
    {
        std::cerr
            << "[AudioCapture] No compatible input device found\n";

        return false;
    }

    const PaDeviceInfo* info =
        Pa_GetDeviceInfo(device);

    if (info == nullptr)
    {
        return false;
    }

    std::cerr
        << "[AudioCapture] Selected input: "
        << info->name
        << '\n';

    PaStreamParameters inputParameters{};
    inputParameters.device = device;
    inputParameters.channelCount = channelCount;
    inputParameters.sampleFormat = paInt16;
    inputParameters.suggestedLatency =
        info->defaultLowInputLatency;

    const PaError error = Pa_OpenStream(
        &stream_,
        &inputParameters,
        nullptr,
        sampleRate,
        framesPerBuffer,
        paNoFlag,
        callback,
        this
    );

    if (error != paNoError)
    {
        std::cerr
            << "[AudioCapture] Pa_OpenStream failed: "
            << Pa_GetErrorText(error)
            << '\n';

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

    if (stream_ == nullptr && !openStream())
    {
        return false;
    }

    writeIndex_.store(
        0,
        std::memory_order_relaxed
    );

    readIndex_.store(
        0,
        std::memory_order_relaxed
    );

    pendingSize_ = 0;

    running_.store(
        true,
        std::memory_order_release
    );

    const PaError error =
        Pa_StartStream(stream_);

    if (error != paNoError)
    {
        running_.store(
            false,
            std::memory_order_release
        );

        wakeCounter_.fetch_add(
            1,
            std::memory_order_relaxed
        );

        wakeCounter_.notify_all();

        std::cerr
            << "[AudioCapture] Pa_StartStream failed: "
            << Pa_GetErrorText(error)
            << '\n';

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

    running_.store(
        false,
        std::memory_order_release
    );

    wakeCounter_.fetch_add(
        1,
        std::memory_order_relaxed
    );

    wakeCounter_.notify_all();

    if (Pa_IsStreamActive(stream_) == 1)
    {
        const PaError error =
            Pa_StopStream(stream_);

        if (error != paNoError)
        {
            std::cerr
                << "[AudioCapture] Pa_StopStream failed: "
                << Pa_GetErrorText(error)
                << '\n';
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
    const int16_t* samples,
    std::size_t frameCount
) -> int
{
    if (!running_.load(std::memory_order_acquire))
    {
        return paComplete;
    }

    for (std::size_t frame = 0; frame < frameCount; ++frame)
    {
        const std::size_t index =
            frame * channelCount;

        const int32_t left =
            samples[index];

        const int32_t right =
            samples[index + 1];

        pendingBuffer_[pendingSize_] =
            static_cast<int16_t>((left + right) / 2);

        ++pendingSize_;

        if (pendingSize_ < framesPerBuffer)
        {
            continue;
        }

        const std::uint64_t writeIndex =
            writeIndex_.load(
                std::memory_order_relaxed
            );

        const std::uint64_t readIndex =
            readIndex_.load(
                std::memory_order_acquire
            );

        if (writeIndex - readIndex >= bufferCount)
        {
            pendingSize_ = 0;
            continue;
        }

        buffers_[writeIndex % bufferCount] =
            pendingBuffer_;

        writeIndex_.store(
            writeIndex + 1,
            std::memory_order_release
        );

        wakeCounter_.fetch_add(
            1,
            std::memory_order_relaxed
        );

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
        const std::uint64_t wake =
            wakeCounter_.load(
                std::memory_order_relaxed
            );

        const std::uint64_t readIndex =
            readIndex_.load(
                std::memory_order_relaxed
            );

        const std::uint64_t writeIndex =
            writeIndex_.load(
                std::memory_order_acquire
            );

        if (readIndex != writeIndex)
        {
            const AudioBuffer& source =
                buffers_[readIndex % bufferCount];

            buffer.assign(
                source.begin(),
                source.end()
            );

            readIndex_.store(
                readIndex + 1,
                std::memory_order_release
            );

            return true;
        }

        if (!running_.load(std::memory_order_acquire))
        {
            return false;
        }

        wakeCounter_.wait(
            wake,
            std::memory_order_relaxed
        );
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
    auto* capture =
        static_cast<AudioCapture*>(userData);

    if (input == nullptr)
    {
        return paContinue;
    }

    return capture->push(
        static_cast<const int16_t*>(input),
        frameCount
    );
}
