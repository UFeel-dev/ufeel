#include "SpeechToText/AudioCapture.hpp"

#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <portaudio.h>
#include <vector>

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

class AudioCaptureTest : public ::testing::Test
{
protected:

    void SetUp() override
    {
        resetFakePortAudioState();
    }

    void TearDown() override
    {
        resetFakePortAudioState();
    }
};

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
    setFakePortAudioInitializeResult(paNotInitialized);
    {
        AudioCapture capture;
        EXPECT_FALSE(capture.start());
    }
}

TEST_F(
    AudioCaptureTest, FailsToStartWhenNoInputDeviceIsAvailable
)
{
    setFakePortAudioDeviceCount(-1);
    {
        AudioCapture capture;
        EXPECT_FALSE(capture.start());
    }

    setFakePortAudioDeviceCount(0);
    {
        AudioCapture capture;
        EXPECT_FALSE(capture.start());
    }
}

TEST_F(
    AudioCaptureTest, SkipsIncompatibleInputDevices
)
{
    setFakePortAudioDeviceInfoAvailable(false);
    {
        AudioCapture capture;
        EXPECT_FALSE(capture.start());
    }

    setFakePortAudioDeviceInfoAvailable(true);
    setFakePortAudioDeviceCapabilities(1, AudioCapture::sampleRate, "Mono input");
    {
        AudioCapture capture;
        EXPECT_FALSE(capture.start());
    }

    setFakePortAudioDeviceCapabilities(2, 44100.0, "Other sample rate");
    setFakePortAudioFormatResult(paInvalidSampleRate);
    {
        AudioCapture capture;
        EXPECT_FALSE(capture.start());
    }
}

TEST_F(
    AudioCaptureTest, FailsWhenStreamCannotBeOpenedOrStarted
)
{
    setFakePortAudioOpenResult(paUnanticipatedHostError);
    {
        AudioCapture capture;
        EXPECT_FALSE(capture.start());
    }

    setFakePortAudioOpenResult(paNoError);
    setFakePortAudioStartResult(paUnanticipatedHostError);
    {
        AudioCapture capture;
        EXPECT_FALSE(capture.start());
    }
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
    setFakePortAudioCallbackInputAvailable(false);
    {
        AudioCapture capture;
        ASSERT_TRUE(capture.start());
        capture.stop();
        EXPECT_FALSE(capture.read(buffer));
    }
}

TEST_F(
    AudioCaptureTest, DropsIncomingAudioWhenTheQueueIsFull
)
{
    setFakePortAudioCallbackFrameCount(
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
    setFakePortAudioStopResult(paUnanticipatedHostError);
    {
        AudioCapture capture;
        ASSERT_TRUE(capture.start());
        EXPECT_NO_THROW(capture.stop());
        EXPECT_NO_THROW(capture.stop());
    }
}
