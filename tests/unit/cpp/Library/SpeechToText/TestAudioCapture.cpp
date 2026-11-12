#include "SpeechToText/AudioCapture.hpp"
#include "TestDoubles.hpp"

#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <portaudio.h>
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
    speech_to_text_test::PortAudioFake::setDeviceCount(-1);
    {
        AudioCapture capture;
        EXPECT_FALSE(capture.start());
    }

    speech_to_text_test::PortAudioFake::setDeviceCount(0);
    {
        AudioCapture capture;
        EXPECT_FALSE(capture.start());
    }
}

TEST_F(
    AudioCaptureTest, SkipsIncompatibleInputDevices
)
{
    speech_to_text_test::PortAudioFake::setDeviceInfoAvailable(false);
    {
        AudioCapture capture;
        EXPECT_FALSE(capture.start());
    }

    speech_to_text_test::PortAudioFake::setDeviceInfoAvailable(true);
    speech_to_text_test::PortAudioFake::setDeviceCapabilities(
        1, AudioCapture::sampleRate, "Mono input"
    );
    {
        AudioCapture capture;
        EXPECT_FALSE(capture.start());
    }

    speech_to_text_test::PortAudioFake::setDeviceCapabilities(2, 44100.0, "Other sample rate");
    speech_to_text_test::PortAudioFake::setFormatResult(paInvalidSampleRate);
    {
        AudioCapture capture;
        EXPECT_FALSE(capture.start());
    }
}

TEST_F(
    AudioCaptureTest, HandlesDeviceInfoDisappearingBeforeStreamOpen
)
{
    speech_to_text_test::PortAudioFake::returnNullDeviceInfoAfterFirstQuery(true);

    AudioCapture capture;

    EXPECT_FALSE(capture.start());
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
    AudioCaptureTest, StartsRepeatedlyAndSelectsNamedHardwareDevice
)
{
    speech_to_text_test::PortAudioFake::setDeviceCapabilities(
        2, AudioCapture::sampleRate, "default (hw:1,0)"
    );

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
        EXPECT_EQ(buffer.front(), 100);
        EXPECT_EQ(buffer.back(), 100);
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
            EXPECT_EQ(buffer.front(), 100);
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
