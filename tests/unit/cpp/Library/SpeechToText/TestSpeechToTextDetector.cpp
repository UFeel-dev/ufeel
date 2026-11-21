#include "Library/SpeechToText/TestSupport/WavAudioSource.hpp"
#include "SpeechToText/AudioCapture.hpp"
#include "SpeechToText/SpeechToTextDetector.hpp"
#include "TestDoubles.hpp"
#include "portaudio.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{

struct FakePaStream
{
    PaStreamCallback* callback = nullptr;
    void* userData = nullptr;
    int channelCount = 0;
    bool active = false;
};

struct FakePortAudioState
{
    PaError initializeResult = paNoError;
    PaError deviceCountError = paNoError;
    std::vector<speech_to_text_test::FakeInputDevice> devices{{}};
    PaDeviceIndex defaultInputDevice = 0;
    bool deviceInfoAvailable = true;
    PaError formatResult = paFormatIsSupported;
    PaError openResult = paNoError;
    PaError startResult = paNoError;
    PaError stopResult = paNoError;
    bool callbackInputAvailable = true;
    unsigned long callbackFrameCount = 1600;
    bool invokeCallbackOnStop = false;

    int initializeCount = 0;
    PaDeviceIndex openedDevice = paNoDevice;
    int openedChannelCount = 0;
    double openedSampleRate = 0.0;
    PaDeviceInfo info{};
};

auto fakePortAudioState() -> FakePortAudioState&
{
    static FakePortAudioState state;
    return state;
}

auto fakeDevice(
    PaDeviceIndex device
) -> const speech_to_text_test::FakeInputDevice*
{
    const auto& devices = fakePortAudioState().devices;
    if (device < 0 || std::cmp_greater_equal(device, devices.size()))
    {
        return nullptr;
    }

    return &devices[static_cast<std::size_t>(device)];
}

auto capturedSamples(
    unsigned long frameCount, int channelCount
) -> std::vector<int16_t>
{
    std::vector<int16_t> samples;
    samples.reserve(frameCount * static_cast<std::size_t>(channelCount));
    for (unsigned long frame = 0; frame < frameCount; ++frame)
    {
        for (int channel = 0; channel < channelCount; ++channel)
        {
            samples.push_back(static_cast<int16_t>(100 + (200 * channel)));
        }
    }

    return samples;
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
    FakePortAudioState& state = fakePortAudioState();
    state.deviceCountError = count < 0 ? count : paNoError;
    state.devices.resize(count < 0 ? 0 : static_cast<std::size_t>(count));
}

void PortAudioFake::setDevices(
    std::vector<FakeInputDevice> devices
)
{
    fakePortAudioState().devices = std::move(devices);
}

void PortAudioFake::setDefaultInputDevice(
    PaDeviceIndex device
)
{
    fakePortAudioState().defaultInputDevice = device;
}

void PortAudioFake::setDeviceInfoAvailable(
    bool available
)
{
    fakePortAudioState().deviceInfoAvailable = available;
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

auto PortAudioFake::initializeCount() -> int
{
    return fakePortAudioState().initializeCount;
}

auto PortAudioFake::openedDevice() -> PaDeviceIndex
{
    return fakePortAudioState().openedDevice;
}

auto PortAudioFake::openedChannelCount() -> int
{
    return fakePortAudioState().openedChannelCount;
}

auto PortAudioFake::openedSampleRate() -> double
{
    return fakePortAudioState().openedSampleRate;
}

} // namespace speech_to_text_test

auto Pa_Initialize() -> PaError
{
    ++fakePortAudioState().initializeCount;
    return fakePortAudioState().initializeResult;
}

auto Pa_Terminate() -> PaError
{
    return paNoError;
}

auto Pa_GetDeviceCount() -> PaDeviceIndex
{
    const FakePortAudioState& state = fakePortAudioState();
    if (state.deviceCountError != paNoError)
    {
        return state.deviceCountError;
    }

    return static_cast<PaDeviceIndex>(state.devices.size());
}

auto Pa_GetDefaultInputDevice() -> PaDeviceIndex
{
    return fakePortAudioState().defaultInputDevice;
}

auto Pa_GetDeviceInfo(
    PaDeviceIndex device
) -> const PaDeviceInfo*
{
    FakePortAudioState& state = fakePortAudioState();
    const auto* fake = fakeDevice(device);
    if (fake == nullptr || !state.deviceInfoAvailable)
    {
        return nullptr;
    }

    state.info = {
        .structVersion = 2,
        .name = fake->name.c_str(),
        .hostApi = 0,
        .maxInputChannels = fake->inputChannels,
        .maxOutputChannels = 0,
        .defaultLowInputLatency = 0.01,
        .defaultLowOutputLatency = 0.1,
        .defaultHighInputLatency = 0.01,
        .defaultHighOutputLatency = 0.1,
        .defaultSampleRate = fake->sampleRates.empty() ? 16000.0 : fake->sampleRates.front(),
    };

    return &state.info;
}

auto Pa_IsFormatSupported(
    const PaStreamParameters* inputParameters,
    const PaStreamParameters* outputParameters,
    double sampleRate
) -> PaError
{
    (void)outputParameters;

    if (fakePortAudioState().formatResult != paFormatIsSupported)
    {
        return fakePortAudioState().formatResult;
    }

    const auto* fake = fakeDevice(inputParameters->device);
    if (fake == nullptr)
    {
        return paInvalidDevice;
    }
    if (inputParameters->channelCount > fake->inputChannels)
    {
        return paInvalidChannelCount;
    }
    if (!fake->sampleRates.empty() &&
        std::ranges::find(fake->sampleRates, sampleRate) == fake->sampleRates.end())
    {
        return paInvalidSampleRate;
    }

    return paFormatIsSupported;
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
    (void)outputParameters;
    (void)framesPerBuffer;
    (void)streamFlags;

    FakePortAudioState& state = fakePortAudioState();
    if (state.openResult != paNoError)
    {
        return state.openResult;
    }

    state.openedDevice = inputParameters->device;
    state.openedChannelCount = inputParameters->channelCount;
    state.openedSampleRate = sampleRate;

    *stream = reinterpret_cast<PaStream*>(new FakePaStream{
        .callback = callback,
        .userData = userData,
        .channelCount = inputParameters->channelCount,
    });

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

    const auto frameCount = fakePortAudioState().callbackFrameCount;
    const auto samples = capturedSamples(frameCount, fakeStream->channelCount);

    if (fakeStream->callback != nullptr)
    {
        const void* input = fakePortAudioState().callbackInputAvailable ? samples.data() : nullptr;
        fakeStream->callback(input, nullptr, frameCount, nullptr, 0, fakeStream->userData);
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
        const auto samples = capturedSamples(1, fakeStream->channelCount);
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
    TestSpeechToTextDetector, RejectsMissingAudioSource
)
{
    EXPECT_THROW(SpeechToTextDetector(modelPath_, nullptr), std::invalid_argument);
}

TEST_F(
    TestSpeechToTextDetector, RecognizesInjectedAudioUntilTextExpires
)
{
    auto audioSource =
        std::make_unique<WavAudioSource>(std::vector<int16_t>(AudioCapture::framesPerBuffer));
    auto* audio = audioSource.get();
    SpeechToTextDetector detector(modelPath_, std::move(audioSource));

    detector.toggle(true);
    ASSERT_TRUE(audio->waitUntilDrained());
    const auto now = SpeechToTextDetector::Clock::now();

    EXPECT_EQ(detector.process(now), "hello");
    EXPECT_TRUE(detector
                    .process(now + SpeechToTextDetector::kTextLifetime + std::chrono::seconds(1))
                    .empty());

    detector.toggle(false);
    EXPECT_EQ(detector.process(), "final sentence");
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

TEST_F(
    TestSpeechToTextDetector, RecognizerFollowsTheCaptureSampleRate
)
{
    SpeechToTextDetector detector(
        modelPath_,
        std::make_unique<WavAudioSource>(
            std::vector<int16_t>(AudioCapture::framesPerBuffer), 48000.0
        )
    );

    detector.toggle(true);
    detector.toggle(false);

    EXPECT_FLOAT_EQ(speech_to_text_test::VoskFake::recognizerSampleRate(), 48000.0F);
}

TEST_F(
    TestSpeechToTextDetector, StaysStoppedWhenTheSampleRateCannotBeRecognized
)
{
    SpeechToTextDetector detector(
        modelPath_, std::make_unique<WavAudioSource>(std::vector<int16_t>{}, 48000.0)
    );
    speech_to_text_test::VoskFake::setRecognizerCreationSucceeds(false);

    detector.toggle(true);
    detector.toggle(false);

    EXPECT_TRUE(detector.process().empty());
}

TEST_F(
    TestSpeechToTextDetector, SelectingAMicrophoneRestartsARunningCapture
)
{
    speech_to_text_test::PortAudioFake::setDevices({{.name = "Built-in"}, {.name = "USB"}});
    SpeechToTextDetector detector(modelPath_);

    ASSERT_EQ(detector.microphones().size(), 2U);

    detector.toggle(true);
    EXPECT_EQ(speech_to_text_test::PortAudioFake::openedDevice(), 0);

    EXPECT_TRUE(detector.selectMicrophone("USB"));
    EXPECT_EQ(speech_to_text_test::PortAudioFake::openedDevice(), 1);

    EXPECT_FALSE(detector.selectMicrophone("Unplugged"));
    EXPECT_EQ(speech_to_text_test::PortAudioFake::openedDevice(), 1);

    detector.toggle(false);
    EXPECT_EQ(detector.process(), "final sentence");
}

TEST_F(
    TestSpeechToTextDetector, SelectingAMicrophoneWhileStoppedDoesNotStartCapture
)
{
    speech_to_text_test::PortAudioFake::setDevices({{.name = "Built-in"}, {.name = "USB"}});
    SpeechToTextDetector detector(modelPath_);

    EXPECT_TRUE(detector.selectMicrophone("USB"));
    EXPECT_EQ(speech_to_text_test::PortAudioFake::openedDevice(), paNoDevice);

    detector.toggle(true);
    EXPECT_EQ(speech_to_text_test::PortAudioFake::openedDevice(), 1);
    detector.toggle(false);
}

} // namespace
