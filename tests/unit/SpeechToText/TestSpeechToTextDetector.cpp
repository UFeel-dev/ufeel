#include "SpeechToText/SpeechToTextDetector.hpp"

#include <cstdint>
#include <gtest/gtest.h>
#include <string>
#include <vector>

void resetFakeVoskForDetector();
void setFakeVoskFinalizedResult(const std::string& result);
void setFakeVoskWaveformResult(int result);

struct FakePaStream
{
    PaStreamCallback* callback = nullptr;
    void* userData = nullptr;
    bool active = false;
};

constexpr int simulatedDeviceCount = 1;
PaDeviceInfo simulatedDeviceInfo{2, "Fake input device", 0, 2, 0, 0.01, 0.1, 0.01, 0.1, 16000.0};


PaError Pa_Initialize()
{
    return paNoError;
}

PaError Pa_Terminate()
{
    return paNoError;
}

PaDeviceIndex Pa_GetDeviceCount()
{
    return simulatedDeviceCount;
}

const PaDeviceInfo* Pa_GetDeviceInfo(
    PaDeviceIndex device
)
{
    if (device < 0 || device >= simulatedDeviceCount)
        return nullptr;

    return &simulatedDeviceInfo;
}

PaError Pa_IsFormatSupported(
    const PaStreamParameters* inputParameters,
    const PaStreamParameters* outputParameters,
    double sampleRate
)
{
    (void)inputParameters;
    (void)outputParameters;
    (void)sampleRate;

    return paFormatIsSupported;
}

PaError Pa_OpenStream(
    PaStream** stream,
    const PaStreamParameters* inputParameters,
    const PaStreamParameters* outputParameters,
    double sampleRate,
    unsigned long framesPerBuffer,
    PaStreamFlags streamFlags,
    PaStreamCallback* callback,
    void* userData
)
{
    (void)inputParameters;
    (void)outputParameters;
    (void)sampleRate;
    (void)framesPerBuffer;
    (void)streamFlags;

    auto* fakeStream = new FakePaStream{};
    fakeStream->callback = callback;
    fakeStream->userData = userData;

    *stream = reinterpret_cast<PaStream*>(fakeStream);

    return paNoError;
}

PaError Pa_StartStream(
    PaStream* stream
)
{
    if (stream == nullptr)
        return paBadStreamPtr;

    auto* fakeStream = reinterpret_cast<FakePaStream*>(stream);
    fakeStream->active = true;

    std::vector<int16_t> samples(1600 * 2, 100);

    if (fakeStream->callback != nullptr)
    {
        fakeStream->callback(samples.data(), nullptr, 1600, nullptr, 0, fakeStream->userData);
    }

    return paNoError;
}

PaError Pa_StopStream(
    PaStream* stream
)
{
    if (stream == nullptr)
        return paBadStreamPtr;

    auto* fakeStream = reinterpret_cast<FakePaStream*>(stream);
    fakeStream->active = false;

    return paNoError;
}

PaError Pa_CloseStream(
    PaStream* stream
)
{
    if (stream == nullptr)
        return paBadStreamPtr;

    delete reinterpret_cast<FakePaStream*>(stream);

    return paNoError;
}

PaError Pa_IsStreamActive(
    PaStream* stream
)
{
    if (stream == nullptr)
        return paBadStreamPtr;

    auto* fakeStream = reinterpret_cast<FakePaStream*>(stream);

    return fakeStream->active ? 1 : 0;
}

const char* Pa_GetErrorText(
    PaError errorCode
)
{
    (void)errorCode;
    return "Fake PortAudio error";
}

class TestSpeechToTextDetector : public ::testing::Test
{
protected:

    void SetUp() override
    {
        resetFakeVoskForDetector();
    }

    const std::string modelPath_ = "fake-model";
};

TEST_F(
    TestSpeechToTextDetector, ConstructorCreatesDetector
)
{
    EXPECT_NO_THROW({ SpeechToTextDetector detector(modelPath_); });
}

TEST_F(
    TestSpeechToTextDetector, ProcessReturnsEmptyTextInitially
)
{
    SpeechToTextDetector detector(modelPath_);

    EXPECT_TRUE(detector.process().empty());
}

TEST_F(
    TestSpeechToTextDetector, ToggleFalseDoesNotCrashBeforeStarting
)
{
    SpeechToTextDetector detector(modelPath_);

    EXPECT_NO_THROW(detector.toggle(false));
    EXPECT_TRUE(detector.process().empty());
}

TEST_F(
    TestSpeechToTextDetector, ToggleTrueStartsAndStopsDetector
)
{
    SpeechToTextDetector detector(modelPath_);

    setFakeVoskFinalizedResult(R"({"text": "final sentence"})");

    detector.toggle(true);
    detector.toggle(false);

    EXPECT_EQ(detector.process(), "final sentence");
}
