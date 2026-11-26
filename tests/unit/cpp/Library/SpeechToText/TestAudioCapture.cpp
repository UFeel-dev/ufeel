#include "SpeechToText/AudioCapture.hpp"
#include "SpeechToText/PulseSources.hpp"
#include "TestDoubles.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <gtest/gtest.h>
#include <optional>
#include <portaudio.h>
#include <utility>
#include <vector>

namespace
{

class AudioCaptureTest : public ::testing::Test
{
protected:

    void SetUp() override
    {
        speech_to_text_test::PortAudioFake::reset();
    }

    void TearDown() override
    {
        speech_to_text_test::PortAudioFake::reset();
    }
};

} // namespace

TEST_F(
    AudioCaptureTest, ReadReturnsFalseBeforeStart
)
{
    AudioCapture capture;
    std::vector<int16_t> buffer;

    EXPECT_FALSE(capture.read(buffer));
    EXPECT_TRUE(buffer.empty());
}

TEST_F(
    AudioCaptureTest, StopWithoutStartingDoesNotCrash
)
{
    AudioCapture capture;

    EXPECT_NO_THROW(capture.stop());
}

TEST_F(
    AudioCaptureTest, RepeatedStopDoesNotCrash
)
{
    AudioCapture capture;

    capture.stop();
    EXPECT_NO_THROW(capture.stop());
}

TEST_F(
    AudioCaptureTest, ReadReturnsFalseWhenStoppedAndNoDataAvailable
)
{
    AudioCapture capture;
    std::vector<int16_t> buffer;

    capture.stop();

    EXPECT_FALSE(capture.read(buffer));
}

TEST_F(
    AudioCaptureTest, MultipleReadsBeforeStartReturnFalse
)
{
    AudioCapture capture;
    std::vector<int16_t> buffer;

    EXPECT_FALSE(capture.read(buffer));
    EXPECT_FALSE(capture.read(buffer));
    EXPECT_FALSE(capture.read(buffer));
}

TEST_F(
    AudioCaptureTest, FailsToStartWhenPortAudioInitializationFails
)
{
    speech_to_text_test::PortAudioFake::setInitializeResult(paNotInitialized);
    {
        AudioCapture capture;
        EXPECT_FALSE(capture.start());
    }
}

TEST_F(
    AudioCaptureTest, FailsToStartWhenNoInputDeviceIsAvailable
)
{
    speech_to_text_test::PortAudioFake::setDevices({});
    AudioCapture capture;

    EXPECT_FALSE(capture.start());
    EXPECT_TRUE(capture.microphones().empty());
}

TEST_F(
    AudioCaptureTest, SkipsIncompatibleInputDevices
)
{
    using speech_to_text_test::PortAudioFake;

    PortAudioFake::setDeviceInfoAvailable(false);
    {
        AudioCapture capture;
        EXPECT_FALSE(capture.start());
    }

    PortAudioFake::setDeviceInfoAvailable(true);
    PortAudioFake::setDevices({{.name = "Output only", .inputChannels = 0}});
    {
        AudioCapture capture;
        EXPECT_FALSE(capture.start());
    }

    PortAudioFake::setDevices({{.name = "Telephone", .sampleRates = {8000.0}}});
    {
        AudioCapture capture;
        EXPECT_FALSE(capture.start());
        EXPECT_TRUE(capture.microphones().empty());
    }

    PortAudioFake::setDevices({{}});
    PortAudioFake::setFormatResult(paInvalidSampleRate);
    {
        AudioCapture capture;
        EXPECT_FALSE(capture.start());
    }
}

TEST_F(
    AudioCaptureTest, CapturesMonoMicrophones
)
{
    speech_to_text_test::PortAudioFake::setDevices({{.name = "Mono", .inputChannels = 1}});
    std::vector<int16_t> buffer;

    AudioCapture capture;
    ASSERT_TRUE(capture.start());
    EXPECT_EQ(speech_to_text_test::PortAudioFake::openedChannelCount(), 1);
    ASSERT_TRUE(capture.read(buffer));
    EXPECT_EQ(buffer.front(), 100);
    capture.stop();
}

TEST_F(
    AudioCaptureTest, PrefersSixteenKilohertzAndFallsBackToNativeRates
)
{
    using speech_to_text_test::PortAudioFake;

    PortAudioFake::setDevices({{.sampleRates = {48000.0, 16000.0}}});
    {
        AudioCapture capture;
        ASSERT_TRUE(capture.start());
        EXPECT_DOUBLE_EQ(PortAudioFake::openedSampleRate(), 16000.0);
        EXPECT_DOUBLE_EQ(capture.sampleRate(), 16000.0);
    }

    PortAudioFake::setDevices({{.sampleRates = {44100.0}}});
    {
        AudioCapture capture;
        ASSERT_TRUE(capture.start());
        EXPECT_DOUBLE_EQ(PortAudioFake::openedSampleRate(), 44100.0);
        EXPECT_DOUBLE_EQ(capture.sampleRate(), 44100.0);
    }
}

TEST_F(
    AudioCaptureTest, ListsUsableMicrophonesAndMarksTheSystemDefault
)
{
    using speech_to_text_test::PortAudioFake;

    PortAudioFake::setDevices(
        {{.name = "Speakers", .inputChannels = 0}, {.name = "Built-in"}, {.name = "USB"}}
    );
    PortAudioFake::setDefaultInputDevice(2);

    AudioCapture capture;
    auto microphones = capture.microphones();

    ASSERT_EQ(microphones.size(), 2U);
    EXPECT_EQ(microphones[0].name, "Built-in");
    EXPECT_FALSE(microphones[0].isDefault);
    EXPECT_EQ(microphones[1].name, "USB");
    EXPECT_TRUE(microphones[1].isDefault);

    ASSERT_TRUE(capture.start());
    EXPECT_EQ(PortAudioFake::openedDevice(), 2);
    capture.stop();

    PortAudioFake::setDefaultInputDevice(0);
    microphones = capture.microphones();
    ASSERT_EQ(microphones.size(), 2U);
    EXPECT_TRUE(microphones[0].isDefault);

    ASSERT_TRUE(capture.start());
    EXPECT_EQ(PortAudioFake::openedDevice(), 1);
}

TEST_F(
    AudioCaptureTest, SelectsMicrophonesByNameFromTheNextStart
)
{
    using speech_to_text_test::PortAudioFake;

    PortAudioFake::setDevices({{.name = "Built-in"}, {.name = "USB"}});

    AudioCapture capture;
    ASSERT_TRUE(capture.start());
    EXPECT_EQ(PortAudioFake::openedDevice(), 0);

    EXPECT_TRUE(capture.selectMicrophone("USB"));
    EXPECT_EQ(PortAudioFake::openedDevice(), 0);
    capture.stop();
    ASSERT_TRUE(capture.start());
    EXPECT_EQ(PortAudioFake::openedDevice(), 1);
    capture.stop();

    EXPECT_FALSE(capture.selectMicrophone("Unplugged"));
    ASSERT_TRUE(capture.start());
    EXPECT_EQ(PortAudioFake::openedDevice(), 1);
    capture.stop();

    EXPECT_TRUE(capture.selectMicrophone(""));
    ASSERT_TRUE(capture.start());
    EXPECT_EQ(PortAudioFake::openedDevice(), 0);
}

TEST_F(
    AudioCaptureTest, FallsBackToTheDefaultWhenTheSelectedMicrophoneDisappears
)
{
    using speech_to_text_test::PortAudioFake;

    PortAudioFake::setDevices({{.name = "Built-in"}, {.name = "USB"}});
    AudioCapture capture;
    ASSERT_TRUE(capture.selectMicrophone("USB"));

    PortAudioFake::setDevices({{.name = "Built-in"}});
    ASSERT_TRUE(capture.start());
    EXPECT_EQ(PortAudioFake::openedDevice(), 0);
}

TEST_F(
    AudioCaptureTest, ListsTheLastScannedMicrophonesWhileCapturing
)
{
    using speech_to_text_test::PortAudioFake;

    PortAudioFake::setDevices({{.name = "Built-in"}, {.name = "USB"}});
    AudioCapture capture;
    ASSERT_TRUE(capture.selectMicrophone("USB"));
    ASSERT_TRUE(capture.start());

    PortAudioFake::setDevices(
        {{.name = "Built-in"}, {.name = "USB", .inputChannels = 0}, {.name = "New"}}
    );
    EXPECT_EQ(capture.microphones().size(), 2U);
    EXPECT_TRUE(capture.selectMicrophone("USB"));

    capture.stop();
    const auto microphones = capture.microphones();
    ASSERT_EQ(microphones.size(), 2U);
    EXPECT_EQ(microphones[1].name, "New");
}

TEST_F(
    AudioCaptureTest, ListsSoundServerSourcesByDescriptionWithoutMonitors
)
{
    using speech_to_text_test::PortAudioFake;

    PortAudioFake::setHostApiType(paPulseAudio);
    PortAudioFake::setDevices({
        {.name = "Default Source", .inputChannels = 32},
        {.name = "Default Sink", .inputChannels = 0},
        {.name = "alsa_input.dmic"},
        {.name = "alsa_output.speaker.monitor"},
        {.name = "bluez_input.headset", .inputChannels = 1},
    });
    PortAudioFake::setDefaultInputDevice(0);
    AudioCapture capture([] -> std::optional<PulseSources> {
        return PulseSources{
            .defaultSource = "bluez_input.headset",
            .sources =
                {
                    {.name = "alsa_input.dmic", .description = "Digital Microphone"},
                    {.name = "alsa_output.speaker.monitor",
                     .description = "Monitor of Speaker",
                     .isMonitor = true},
                    {.name = "bluez_input.headset", .description = "Headset"},
                },
        };
    });

    const auto microphones = capture.microphones();

    ASSERT_EQ(microphones.size(), 2U);
    EXPECT_EQ(microphones[0].id, "alsa_input.dmic");
    EXPECT_EQ(microphones[0].name, "Digital Microphone");
    EXPECT_FALSE(microphones[0].isDefault);
    EXPECT_EQ(microphones[1].id, "bluez_input.headset");
    EXPECT_EQ(microphones[1].name, "Headset");
    EXPECT_TRUE(microphones[1].isDefault);

    ASSERT_TRUE(capture.start());
    EXPECT_EQ(PortAudioFake::openedDevice(), 0);
    EXPECT_EQ(PortAudioFake::openedChannelCount(), 2);
    EXPECT_DOUBLE_EQ(PortAudioFake::openedSampleRate(), 16000.0);
    capture.stop();

    EXPECT_FALSE(capture.selectMicrophone("Digital Microphone"));
    EXPECT_FALSE(capture.selectMicrophone("alsa_output.speaker.monitor"));
    ASSERT_TRUE(capture.selectMicrophone("alsa_input.dmic"));
    ASSERT_TRUE(capture.start());
    EXPECT_EQ(PortAudioFake::openedDevice(), 2);
}

TEST_F(
    AudioCaptureTest, WatchesSoundServerChangesOnceItsSourcesAreListed
)
{
    using speech_to_text_test::PortAudioFake;

    PortAudioFake::setHostApiType(paPulseAudio);
    PortAudioFake::setDevices({{.name = "Default Source"}, {.name = "alsa_input.dmic"}});
    int watchers = 0;
    bool changed = false;
    AudioCapture capture(
        [] -> std::optional<PulseSources> {
            return PulseSources{
                .sources = {{.name = "alsa_input.dmic", .description = "Microphone"}}};
        },
        [&watchers, &changed] -> std::function<bool()> {
            ++watchers;
            return [&changed] -> bool { return std::exchange(changed, false); };
        }
    );

    changed = true;
    EXPECT_FALSE(capture.microphonesChanged());

    static_cast<void>(capture.microphones());
    static_cast<void>(capture.microphones());
    EXPECT_EQ(watchers, 1);
    EXPECT_TRUE(capture.microphonesChanged());
    EXPECT_FALSE(capture.microphonesChanged());
}

TEST_F(
    AudioCaptureTest, ListsCoreAudioDevicesAndWatchesTheirChanges
)
{
    using speech_to_text_test::PortAudioFake;

    PortAudioFake::setHostApiType(paCoreAudio);
    PortAudioFake::setDevices({{.name = "MacBook Pro Microphone"}, {.name = "USB Microphone"}});
    PortAudioFake::setDefaultInputDevice(1);
    int queries = 0;
    int watchers = 0;
    bool changed = false;
    AudioCapture capture(
        [&queries] -> std::optional<PulseSources> {
            ++queries;
            return std::nullopt;
        },
        [&watchers, &changed] -> std::function<bool()> {
            ++watchers;
            return [&changed] -> bool { return std::exchange(changed, false); };
        }
    );

    changed = true;
    EXPECT_FALSE(capture.microphonesChanged());

    const auto microphones = capture.microphones();
    static_cast<void>(capture.microphones());

    ASSERT_EQ(microphones.size(), 2U);
    EXPECT_EQ(microphones[0].id, "MacBook Pro Microphone");
    EXPECT_EQ(microphones[0].name, "MacBook Pro Microphone");
    EXPECT_FALSE(microphones[0].isDefault);
    EXPECT_EQ(microphones[1].id, "USB Microphone");
    EXPECT_TRUE(microphones[1].isDefault);
    EXPECT_EQ(queries, 0);
    EXPECT_EQ(watchers, 1);
    EXPECT_TRUE(capture.microphonesChanged());
    EXPECT_FALSE(capture.microphonesChanged());

    ASSERT_TRUE(capture.start());
    EXPECT_EQ(PortAudioFake::openedDevice(), 1);
}

TEST_F(
    AudioCaptureTest, ListsDeviceNamesWithoutSoundServer
)
{
    using speech_to_text_test::PortAudioFake;

    PortAudioFake::setHostApiType(paPulseAudio);
    PortAudioFake::setDevices({{.name = "hw:0,0"}});
    AudioCapture capture([] -> std::optional<PulseSources> { return std::nullopt; });

    const auto microphones = capture.microphones();

    ASSERT_EQ(microphones.size(), 1U);
    EXPECT_EQ(microphones[0].id, "hw:0,0");
    EXPECT_EQ(microphones[0].name, "hw:0,0");
    EXPECT_TRUE(microphones[0].isDefault);
}

TEST_F(
    AudioCaptureTest, QueriesTheSoundServerOnlyThroughItsHostApi
)
{
    int queries = 0;
    int watchers = 0;
    AudioCapture capture(
        [&queries] -> std::optional<PulseSources> {
            ++queries;
            return std::nullopt;
        },
        [&watchers] -> std::function<bool()> {
            ++watchers;
            return [] -> bool { return true; };
        }
    );

    EXPECT_EQ(capture.microphones().size(), 1U);
    EXPECT_EQ(queries, 0);
    EXPECT_EQ(watchers, 0);
    EXPECT_FALSE(capture.microphonesChanged());
}

TEST_F(
    AudioCaptureTest, RescansDevicesOnlyWhileStopped
)
{
    using speech_to_text_test::PortAudioFake;

    AudioCapture capture;
    int initialized = PortAudioFake::initializeCount();

    static_cast<void>(capture.microphones());
    EXPECT_EQ(PortAudioFake::initializeCount(), initialized + 1);

    ASSERT_TRUE(capture.start());
    initialized = PortAudioFake::initializeCount();
    static_cast<void>(capture.microphones());
    EXPECT_TRUE(capture.selectMicrophone("Fake input device"));
    EXPECT_FALSE(capture.selectMicrophone("Unplugged"));
    EXPECT_EQ(PortAudioFake::initializeCount(), initialized);
}

TEST_F(
    AudioCaptureTest, FailsWhenStreamCannotBeOpenedOrStarted
)
{
    speech_to_text_test::PortAudioFake::setOpenResult(paUnanticipatedHostError);
    {
        AudioCapture capture;
        EXPECT_FALSE(capture.start());
    }

    speech_to_text_test::PortAudioFake::setOpenResult(paNoError);
    speech_to_text_test::PortAudioFake::setStartResult(paUnanticipatedHostError);
    {
        AudioCapture capture;
        EXPECT_FALSE(capture.start());
    }
}

TEST_F(
    AudioCaptureTest, StartsRepeatedly
)
{
    AudioCapture capture;

    ASSERT_TRUE(capture.start());
    EXPECT_TRUE(capture.start());
    capture.stop();
}

TEST_F(
    AudioCaptureTest, ReadsStereoDownmixedAudioAndHandlesCallbackWithoutInput
)
{
    std::vector<int16_t> buffer;
    {
        AudioCapture capture;
        ASSERT_TRUE(capture.start());
        ASSERT_TRUE(capture.read(buffer));
        ASSERT_EQ(buffer.size(), AudioCapture::framesPerBuffer);
        EXPECT_EQ(buffer.front(), 200);
        EXPECT_EQ(buffer.back(), 200);
        capture.stop();
    }
    speech_to_text_test::PortAudioFake::setCallbackInputAvailable(false);
    {
        AudioCapture capture;
        ASSERT_TRUE(capture.start());
        capture.stop();
        EXPECT_FALSE(capture.read(buffer));
    }
}

TEST_F(
    AudioCaptureTest, IgnoresCallbackAudioAfterCaptureStops
)
{
    speech_to_text_test::PortAudioFake::invokeCallbackOnStop(true);

    AudioCapture capture;
    ASSERT_TRUE(capture.start());
    capture.stop();
}

TEST_F(
    AudioCaptureTest, DropsIncomingAudioWhenTheQueueIsFull
)
{
    speech_to_text_test::PortAudioFake::setCallbackFrameCount(
        static_cast<unsigned long>(AudioCapture::framesPerBuffer * (AudioCapture::bufferCount + 1))
    );

    std::vector<int16_t> buffer;
    {
        AudioCapture capture;
        ASSERT_TRUE(capture.start());
        for (std::size_t index = 0; index < AudioCapture::bufferCount; ++index)
        {
            ASSERT_TRUE(capture.read(buffer));
            ASSERT_EQ(buffer.size(), AudioCapture::framesPerBuffer);
            EXPECT_EQ(buffer.front(), 200);
        }
        capture.stop();
        EXPECT_FALSE(capture.read(buffer));
    }
}

TEST_F(
    AudioCaptureTest, HandlesStopErrorsAndRepeatedStop
)
{
    speech_to_text_test::PortAudioFake::setStopResult(paUnanticipatedHostError);
    {
        AudioCapture capture;
        ASSERT_TRUE(capture.start());
        EXPECT_NO_THROW(capture.stop());
        EXPECT_NO_THROW(capture.stop());
    }
}
