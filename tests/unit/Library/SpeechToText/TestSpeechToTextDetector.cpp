#include "SpeechToText/SpeechToTextDetector.hpp"
#include "TestDoubles.hpp"
#include "portaudio.h"

#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

namespace
{

struct FakePaStream
{
    PaStreamCallback* callback = nullptr;
    void* userData = nullptr;
    bool active = false;
};

struct FakePortAudioState
{
    PaError initializeResult = paNoError;
    PaDeviceIndex deviceCount = 1;
    bool deviceInfoAvailable = true;
    bool nullDeviceInfoAfterFirstQuery = false;
    std::size_t deviceInfoQueryCount = 0;
    int inputChannels = 2;
    double deviceSampleRate = 16000.0;
    const char* deviceName = "Fake input device";
    PaError formatResult = paFormatIsSupported;
    PaError openResult = paNoError;
    PaError startResult = paNoError;
    PaError stopResult = paNoError;
    bool callbackInputAvailable = true;
    unsigned long callbackFrameCount = 1600;
    bool invokeCallbackOnStop = false;
};

auto fakePortAudioState() -> FakePortAudioState&
{
    static FakePortAudioState state;
    return state;
}

auto simulatedDeviceInfo() -> PaDeviceInfo&
{
    static PaDeviceInfo info{
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

    return info;
}

} // namespace

namespace speech_to_text_test
{

void PortAudioFake::reset()
{
    fakePortAudioState() = {};
}

void PortAudioFake::setInitializeResult(
    PaError error
)
{
    fakePortAudioState().initializeResult = error;
}

void PortAudioFake::setDeviceCount(
    PaDeviceIndex count
)
{
    fakePortAudioState().deviceCount = count;
}

void PortAudioFake::setDeviceInfoAvailable(
    bool available
)
{
    fakePortAudioState().deviceInfoAvailable = available;
}

void PortAudioFake::returnNullDeviceInfoAfterFirstQuery(
    bool enabled
)
{
    fakePortAudioState().nullDeviceInfoAfterFirstQuery = enabled;
}

void PortAudioFake::setDeviceCapabilities(
    int channels, double sampleRate, const char* name
)
{
    fakePortAudioState().inputChannels = channels;
    fakePortAudioState().deviceSampleRate = sampleRate;
    fakePortAudioState().deviceName = name;
}

void PortAudioFake::setFormatResult(
    PaError error
)
{
    fakePortAudioState().formatResult = error;
}

void PortAudioFake::setOpenResult(
    PaError error
)
{
    fakePortAudioState().openResult = error;
}

void PortAudioFake::setStartResult(
    PaError error
)
{
    fakePortAudioState().startResult = error;
}

void PortAudioFake::setStopResult(
    PaError error
)
{
    fakePortAudioState().stopResult = error;
}

void PortAudioFake::setCallbackInputAvailable(
    bool available
)
{
    fakePortAudioState().callbackInputAvailable = available;
}

void PortAudioFake::setCallbackFrameCount(
    unsigned long frameCount
)
{
    fakePortAudioState().callbackFrameCount = frameCount;
}

void PortAudioFake::invokeCallbackOnStop(
    bool enabled
)
{
    fakePortAudioState().invokeCallbackOnStop = enabled;
}

} // namespace speech_to_text_test

auto Pa_Initialize() -> PaError
{
    return fakePortAudioState().initializeResult;
}

auto Pa_Terminate() -> PaError
{
    return paNoError;
}

auto Pa_GetDeviceCount() -> PaDeviceIndex
{
    return fakePortAudioState().deviceCount;
}

auto Pa_GetDeviceInfo(
    PaDeviceIndex device
) -> const PaDeviceInfo*
{
    FakePortAudioState& state = fakePortAudioState();
    const bool returnNull = state.nullDeviceInfoAfterFirstQuery && state.deviceInfoQueryCount > 0;
    ++state.deviceInfoQueryCount;
    if (device < 0 || device >= state.deviceCount || !state.deviceInfoAvailable || returnNull)
    {
        return nullptr;
    }

    PaDeviceInfo& info = simulatedDeviceInfo();
    info.maxInputChannels = fakePortAudioState().inputChannels;
    info.name = fakePortAudioState().deviceName;
    info.defaultSampleRate = fakePortAudioState().deviceSampleRate;

    return &info;
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

    return fakePortAudioState().formatResult;
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

    if (fakePortAudioState().openResult != paNoError)
    {
        return fakePortAudioState().openResult;
    }

    auto* fakeStream = new FakePaStream{};
    fakeStream->callback = callback;
    fakeStream->userData = userData;

    *stream = reinterpret_cast<PaStream*>(fakeStream);

    return paNoError;
}

auto Pa_StartStream(
    PaStream* stream
) -> PaError
{
    if (stream == nullptr)
    {
        return paBadStreamPtr;
    }

    auto* fakeStream = reinterpret_cast<FakePaStream*>(stream);
    if (fakePortAudioState().startResult != paNoError)
    {
        return fakePortAudioState().startResult;
    }

    fakeStream->active = true;

    std::vector<int16_t> samples(fakePortAudioState().callbackFrameCount * 2, 100);

    if (fakeStream->callback != nullptr)
    {
        const void* input = fakePortAudioState().callbackInputAvailable ? samples.data() : nullptr;
        fakeStream->callback(
            input,
            nullptr,
            fakePortAudioState().callbackFrameCount,
            nullptr,
            0,
            fakeStream->userData
        );
    }

    return paNoError;
}

auto Pa_StopStream(
    PaStream* stream
) -> PaError
{
    if (stream == nullptr)
    {
        return paBadStreamPtr;
    }

    auto* fakeStream = reinterpret_cast<FakePaStream*>(stream);
    fakeStream->active = false;

    if (fakePortAudioState().invokeCallbackOnStop && fakeStream->callback != nullptr)
    {
        std::vector<int16_t> samples(2, 100);
        fakeStream->callback(samples.data(), nullptr, 1, nullptr, 0, fakeStream->userData);
    }

    return fakePortAudioState().stopResult;
}

auto Pa_CloseStream(
    PaStream* stream
) -> PaError
{
    if (stream == nullptr)
    {
        return paBadStreamPtr;
    }

    delete reinterpret_cast<FakePaStream*>(stream);

    return paNoError;
}

auto Pa_IsStreamActive(
    PaStream* stream
) -> PaError
{
    if (stream == nullptr)
    {
        return paBadStreamPtr;
    }

    auto* fakeStream = reinterpret_cast<FakePaStream*>(stream);

    return fakeStream->active ? 1 : 0;
}

auto Pa_GetErrorText(
    PaError errorCode
) -> const char*
{
    (void)errorCode;
    return "Fake PortAudio error";
}

namespace
{

class TestSpeechToTextDetector : public ::testing::Test
{
protected:

    void SetUp() override
    {
        speech_to_text_test::VoskFake::reset();
        speech_to_text_test::PortAudioFake::reset();
    }

    const std::string modelPath_ = "fake-model";
};

TEST_F(
    TestSpeechToTextDetector, ConstructorCreatesDetector
)
{
    EXPECT_NO_THROW(const SpeechToTextDetector detector(modelPath_));
}

TEST_F(
    TestSpeechToTextDetector, ProcessReturnsEmptyTextInitially
)
{
    const SpeechToTextDetector detector(modelPath_);

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
    TestSpeechToTextDetector, StartFailureLeavesDetectorStopped
)
{
    speech_to_text_test::PortAudioFake::setInitializeResult(paNotInitialized);
    SpeechToTextDetector detector(modelPath_);

    detector.toggle(true);
    detector.toggle(false);

    EXPECT_TRUE(detector.process().empty());
}

TEST_F(
    TestSpeechToTextDetector, ToggleTrueStartsAndStopsDetector
)
{
    SpeechToTextDetector detector(modelPath_);

    speech_to_text_test::VoskFake::setFinalizedResult(R"({"text": "final sentence"})");

    detector.toggle(true);
    detector.toggle(false);

    EXPECT_EQ(detector.process(), "final sentence");
}

TEST_F(
    TestSpeechToTextDetector, EmptyRecognitionAndFinalizationDoNotReplaceText
)
{
    SpeechToTextDetector detector(modelPath_);
    speech_to_text_test::VoskFake::setPartialResult(R"({"partial": ""})");
    speech_to_text_test::VoskFake::setFinalizedResult(R"({"text": ""})");

    detector.toggle(true);
    detector.toggle(false);

    EXPECT_TRUE(detector.process().empty());
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

} // namespace
