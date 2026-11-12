#include "DataProcessor.hpp"
#include "EyeTracking/EyeTrackingDetector.hpp"
#include "HeartRate/HeartRateSource.hpp"
#include "HeartRate/SensorHeartRateSource.hpp"
#include "HeartRate/UFeelHeartRateStatus.hpp"
#include "HeartRate/WatchHeartRateSource.hpp"
#include "Library/HeartRate/TestSupport/HeartRateTestHelpers.hpp"
#include "Library/HeartRate/TestSupport/LocalHttpServer.hpp"
#include "Library/SpeechToText/TestSupport/WavAudioSource.hpp"
#include "Library/TestSupport/Fixtures.hpp"
#include "SpeechToText/AudioCapture.hpp"
#include "SpeechToText/AudioSource.hpp"
#include "opencv2/core/mat.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#ifndef UFEEL_TEST_MODEL_DIR
#define UFEEL_TEST_MODEL_DIR ""
#endif // UFEEL_TEST_MODEL_DIR
#ifndef UFEEL_TEST_SPEECH_MODEL_DIR
#define UFEEL_TEST_SPEECH_MODEL_DIR ""
#endif // UFEEL_TEST_SPEECH_MODEL_DIR

namespace
{

auto createProcessor(
    std::function<bool(cv::Mat&)> frameReader,
    std::unique_ptr<AudioSource> audioSource,
    std::vector<std::unique_ptr<HeartRateSource>> heartRateSources
) -> DataProcessor
{
    return {
        UFEEL_TEST_MODEL_DIR,
        UFEEL_TEST_SPEECH_MODEL_DIR,
        DataProcessorDevices{
            .frameReader = std::move(frameReader),
            .audioSource = std::move(audioSource),
            .heartRateSources = std::move(heartRateSources),
        },
    };
}

auto watchSources() -> std::vector<std::unique_ptr<HeartRateSource>>
{
    std::vector<std::unique_ptr<HeartRateSource>> sources;
    sources.push_back(std::make_unique<WatchHeartRateSource>(0));
    return sources;
}

auto createProcessor(
    std::vector<cv::Mat> frames
) -> DataProcessor
{
    return createProcessor(
        frameSequence(std::move(frames)),
        std::make_unique<WavAudioSource>(std::vector<int16_t>{}),
        watchSources()
    );
}

auto transcribe(
    std::vector<int16_t> samples
) -> SpeechToTextResult
{
    auto audioSource = std::make_unique<WavAudioSource>(std::move(samples));
    auto* audio = audioSource.get();
    auto processor = createProcessor(frameSequence({}), std::move(audioSource), watchSources());

    processor.toggleSpeechToText(true);
    EXPECT_TRUE(audio->waitUntilDrained());
    EXPECT_EQ(processor.updateSpeechToText(), DataProcessorStatus::Ok);
    return processor.getSpeechToText();
}

auto waitForHeartRate(
    DataProcessor& processor, std::uint32_t bpm
) -> HeartRateResult
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (std::chrono::steady_clock::now() < deadline)
    {
        processor.updateHeartRate();
        if (processor.getHeartRate().status == HeartRateStatus::Valid &&
            processor.getHeartRate().bpm == bpm)
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return processor.getHeartRate();
}

} // namespace

TEST(
    DataProcessorFixtureTest, FaceFrameReportsHappyEmotionAndUncalibratedGaze
)
{
    auto processor = createProcessor({loadFaceFrame()});

    ASSERT_EQ(processor.update(Emotions | EyeTracking), DataProcessorStatus::Ok);

    EXPECT_EQ(processor.getEmotions().status, EmotionStatus::Valid);
    EXPECT_TRUE(processor.getEmotions().emotions.contains("happy"));
    EXPECT_EQ(processor.getEyeTracking().status, EyeTrackingStatus::NotCalibrated);
}

TEST(
    DataProcessorFixtureTest, GreyscaleFaceFrameIsProcessedLikeColourFrame
)
{
    auto processor = createProcessor({makeGreyscaleFaceFrame()});

    ASSERT_EQ(processor.update(Emotions | EyeTracking), DataProcessorStatus::Ok);

    EXPECT_EQ(processor.getEmotions().status, EmotionStatus::Valid);
    EXPECT_EQ(processor.getEyeTracking().status, EyeTrackingStatus::NotCalibrated);
}

TEST(
    DataProcessorFixtureTest, FramesWithoutUsableFaceReportFaceUnavailable
)
{
    auto processor = createProcessor({makeBlankFrame(), makeSmallFaceFrame()});

    for (int frame = 0; frame < 2; ++frame)
    {
        ASSERT_EQ(processor.update(Emotions | EyeTracking), DataProcessorStatus::Ok);
        EXPECT_EQ(processor.getEmotions().status, EmotionStatus::FaceUnavailable);
        EXPECT_TRUE(processor.getEmotions().emotions.empty());
        EXPECT_EQ(processor.getEyeTracking().status, EyeTrackingStatus::FaceUnavailable);
    }

    EXPECT_EQ(processor.update(Emotions | EyeTracking), DataProcessorStatus::FrameUnavailable);
    EXPECT_EQ(processor.getEmotions().status, EmotionStatus::FrameUnavailable);
    EXPECT_EQ(processor.getEyeTracking().status, EyeTrackingStatus::FrameUnavailable);
}

TEST(
    DataProcessorFixtureTest, TranscribesSpokenSentence
)
{
    const auto result = transcribe(readWav(resourcePath("turn_on_the_light.wav")));

    EXPECT_EQ(result.status, SpeechToTextStatus::Valid);
    EXPECT_EQ(result.text, "turn on the light");
}

TEST(
    DataProcessorFixtureTest, ReportsNoSpeechForSilence
)
{
    const auto result =
        transcribe(std::vector<int16_t>(static_cast<std::size_t>(AudioCapture::sampleRate) * 2));

    EXPECT_EQ(result.status, SpeechToTextStatus::NoSpeech);
    EXPECT_TRUE(result.text.empty());
}

TEST(
    DataProcessorFixtureTest, ReadsHeartRateFromLocalSensor
)
{
    const LocalHttpServer sensor(200, R"({"bpm": 72})");
    ASSERT_TRUE(sensor.isReady());
    std::vector<std::unique_ptr<HeartRateSource>> sources;
    sources.push_back(std::make_unique<SensorHeartRateSource>(sensor.url()));
    auto processor = createProcessor(
        frameSequence({}),
        std::make_unique<WavAudioSource>(std::vector<int16_t>{}),
        std::move(sources)
    );

    const auto result = waitForHeartRate(processor, 72);

    EXPECT_EQ(result.status, HeartRateStatus::Valid);
    EXPECT_EQ(result.bpm, 72U);
}

TEST(
    DataProcessorFixtureTest, ReadsHeartRateFromWatchPacket
)
{
    auto watch = std::make_unique<WatchHeartRateSource>(0);
    const auto port = watch->port();
    ASSERT_NE(port, 0U);
    std::vector<std::unique_ptr<HeartRateSource>> sources;
    sources.push_back(std::move(watch));
    auto processor = createProcessor(
        frameSequence({}),
        std::make_unique<WavAudioSource>(std::vector<int16_t>{}),
        std::move(sources)
    );

    ASSERT_TRUE(sendHeartRatePacket(port, "84", 2));
    const auto result = waitForHeartRate(processor, 84);

    EXPECT_EQ(result.status, HeartRateStatus::Valid);
    EXPECT_EQ(result.bpm, 84U);
}
