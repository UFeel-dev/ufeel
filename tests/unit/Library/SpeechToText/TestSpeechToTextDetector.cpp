#include "SpeechToText/SpeechToTextDetector.hpp"

#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <portaudio.h>
#include <string>
#include <vector>

void resetFakeVoskForDetector();
void setFakeVoskFinalizedResult(const std::string& result);
void setFakeVoskWaveformResult(int result);
void resetFakePortAudioState();
void setFakePortAudioInitializeResult(PaError error);
void setFakePortAudioDeviceCount(PaDeviceIndex count);
void setFakePortAudioDeviceInfoAvailable(bool available);
void setFakePortAudioDeviceCapabilities(int channels, double sampleRate, const char* name);
void setFakePortAudioFormatResult(PaError error);
void setFakePortAudioOpenResult(PaError error);
void setFakePortAudioStartResult(PaError error);
void setFakePortAudioStopResult(PaError error);
void setFakePortAudioCallbackInputAvailable(bool available);
void setFakePortAudioCallbackFrameCount(unsigned long frameCount);

struct FakePaStream
{
    PaStreamCallback* callback = nullptr;
    void* userData = nullptr;
    bool active = false;
};

namespace
{

struct FakePortAudioState
{
    PaError initializeResult = paNoError;
    PaDeviceIndex deviceCount = 1;
    bool deviceInfoAvailable = true;
    int inputChannels = 2;
    double deviceSampleRate = 16000.0;
    const char* deviceName = "Fake input device";
    PaError formatResult = paFormatIsSupported;
    PaError openResult = paNoError;
    PaError startResult = paNoError;
    PaError stopResult = paNoError;
    bool callbackInputAvailable = true;
    unsigned long callbackFrameCount = 1600;
};

FakePortAudioState fakePortAudio;
PaDeviceInfo simulatedDeviceInfo{
    .structVersion = 2,
    .name = "Fake input device",
    .hostApi = 0,
    .maxInputChannels = 2,
    .maxOutputChannels = 0,
    .defaultLowInputLatency = 0.01,
    .defaultLowOutputLatency = 0.1,
    .defaultHighInputLatency = 0.01,
    .defaultHighOutputLatency = 0.1,
    .defaultSampleRate = 16000.0,
};

} // namespace

void resetFakePortAudioState()
{
    fakePortAudio = {};
}

void setFakePortAudioInitializeResult(
    PaError error
)
{
    fakePortAudio.initializeResult = error;
}

void setFakePortAudioDeviceCount(
    PaDeviceIndex count
)
{
    fakePortAudio.deviceCount = count;
}

void setFakePortAudioDeviceInfoAvailable(
    bool available
)
{
    fakePortAudio.deviceInfoAvailable = available;
}

void setFakePortAudioDeviceCapabilities(
    int channels, double sampleRate, const char* name
)
{
    fakePortAudio.inputChannels = channels;
    fakePortAudio.deviceSampleRate = sampleRate;
    fakePortAudio.deviceName = name;
}

void setFakePortAudioFormatResult(
    PaError error
)
{
    fakePortAudio.formatResult = error;
}

void setFakePortAudioOpenResult(
    PaError error
)
{
    fakePortAudio.openResult = error;
}

void setFakePortAudioStartResult(
    PaError error
)
{
    fakePortAudio.startResult = error;
}

void setFakePortAudioStopResult(
    PaError error
)
{
    fakePortAudio.stopResult = error;
}

void setFakePortAudioCallbackInputAvailable(
    bool available
)
{
    fakePortAudio.callbackInputAvailable = available;
}

void setFakePortAudioCallbackFrameCount(
    unsigned long frameCount
)
{
    fakePortAudio.callbackFrameCount = frameCount;
}

auto Pa_Initialize() -> PaError
{
    return fakePortAudio.initializeResult;
}

auto Pa_Terminate() -> PaError
{
    return paNoError;
}

auto Pa_GetDeviceCount() -> PaDeviceIndex
{
    return fakePortAudio.deviceCount;
}

auto Pa_GetDeviceInfo(
    PaDeviceIndex device
) -> const PaDeviceInfo*
{
    if (device < 0 || device >= fakePortAudio.deviceCount || !fakePortAudio.deviceInfoAvailable)
        return nullptr;

    simulatedDeviceInfo.maxInputChannels = fakePortAudio.inputChannels;
    simulatedDeviceInfo.name = fakePortAudio.deviceName;
    simulatedDeviceInfo.defaultSampleRate = fakePortAudio.deviceSampleRate;

    return &simulatedDeviceInfo;
}

auto Pa_IsFormatSupported(
    const PaStreamParameters* inputParameters,
    const PaStreamParameters* outputParameters,
    double sampleRate
) -> PaError
{
    (void)inputParameters;
    (void)outputParameters;
    (void)sampleRate;

    return fakePortAudio.formatResult;
}

auto Pa_OpenStream(
    PaStream** stream,
    const PaStreamParameters* inputParameters,
    const PaStreamParameters* outputParameters,
    double sampleRate,
    unsigned long framesPerBuffer,
    PaStreamFlags streamFlags,
    PaStreamCallback* callback,
    void* userData
) -> PaError
{
    (void)inputParameters;
    (void)outputParameters;
    (void)sampleRate;
    (void)framesPerBuffer;
    (void)streamFlags;

    if (fakePortAudio.openResult != paNoError)
        return fakePortAudio.openResult;

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
    if (fakePortAudio.startResult != paNoError)
        return fakePortAudio.startResult;

    fakeStream->active = true;

    std::vector<int16_t> samples(fakePortAudio.callbackFrameCount * 2, 100);

    if (fakeStream->callback != nullptr)
    {
        const void* input = fakePortAudio.callbackInputAvailable ? samples.data() : nullptr;
        fakeStream->callback(
            input, nullptr, fakePortAudio.callbackFrameCount, nullptr, 0, fakeStream->userData
        );
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

    return fakePortAudio.stopResult;
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
        resetFakePortAudioState();
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

TEST_F(
    TestSpeechToTextDetector, ToggleTrueCalledMultipleTimesDoesNotCrash
)
{
    SpeechToTextDetector detector(modelPath_);

    EXPECT_NO_THROW({
        detector.toggle(true);
        detector.toggle(true);
        detector.toggle(false);
    });

    EXPECT_EQ(detector.process(), "final sentence");
}

TEST_F(
    TestSpeechToTextDetector, ToggleFalseCalledMultipleTimesDoesNotCrash
)
{
    SpeechToTextDetector detector(modelPath_);

    EXPECT_NO_THROW({
        detector.toggle(true);
        detector.toggle(false);
        detector.toggle(false);
        detector.toggle(false);
    });
}

TEST_F(
    TestSpeechToTextDetector, DestructorStopsDetector
)
{
    EXPECT_NO_THROW({
        auto detector = std::make_unique<SpeechToTextDetector>(modelPath_);
        detector->toggle(true);
        detector.reset();
    });
}
