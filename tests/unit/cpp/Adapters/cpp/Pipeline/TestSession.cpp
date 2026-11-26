#include "Adapters/cpp/TestSupport/Eventually.hpp"
#include "Adapters/cpp/TestSupport/FakeProcessor.hpp"
#include "DataProcessor.hpp"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"
#include "ufeel/Model/Detection.hpp"
#include "ufeel/Model/EmotionResult.hpp"
#include "ufeel/Model/Flags.hpp"
#include "ufeel/Model/HeartRateResult.hpp"
#include "ufeel/Model/SpeechToTextResult.hpp"
#include "ufeel/Native/Processor.hpp"
#include "ufeel/Pipeline/Session.hpp"

#include <atomic>
#include <future>
#include <gtest/gtest.h>
#include <stdexcept>
#include <thread>
#include <utility>

TEST(
    SessionTest, PublishesPinnedFeatureResults
)
{
    auto [backend, processor] = makeFakeProcessor();
    ufeel::Session session(std::move(processor));
    session.demand().pin(ufeel::DetectionType::HeartRate);

    session.start();

    ASSERT_TRUE(eventually([&session] -> bool { return session.latest()->heartRate.isValid(); }));
    EXPECT_EQ(session.latest()->heartRate.bpm, 73U);
    EXPECT_TRUE(session.latest()->message.empty());
}

TEST(
    SessionTest, RunsImageFeaturesOnCapturedFrames
)
{
    auto [backend, processor] = makeFakeProcessor();
    backend->frame = cv::Mat(4, 4, CV_8UC3, cv::Scalar::all(10));
    std::atomic<int> capturedFrames = 0;
    ufeel::Session session(
        std::move(processor),
        {.onFrameCaptured = [&capturedFrames](ufeel::Processor&) -> void { ++capturedFrames; }}
    );
    session.demand().pin(ufeel::DetectionType::Emotions);

    session.start();

    ASSERT_TRUE(eventually([&session] -> bool { return session.latest()->emotions.isValid(); }));
    EXPECT_TRUE(session.latest()->frameAvailable);
    EXPECT_FLOAT_EQ(session.latest()->emotions.scores.at("joy"), 0.75F);
    EXPECT_GT(capturedFrames.load(), 0);
}

TEST(
    SessionTest, ReportsAnUnavailableCamera
)
{
    auto [backend, processor] = makeFakeProcessor();
    backend->updateFrameStatus = DataProcessorStatus::FrameUnavailable;
    ufeel::Session session(std::move(processor));
    session.demand().pin(ufeel::DetectionType::Emotions);

    session.start();

    ASSERT_TRUE(eventually([&session] -> bool {
        return session.latest()->message == "Camera frame unavailable";
    }));
    EXPECT_FALSE(session.latest()->frameAvailable);
    EXPECT_EQ(session.latest()->emotions.status, ufeel::EmotionStatus::FrameUnavailable);
}

TEST(
    SessionTest, StopsRequestingUnsupportedFeatures
)
{
    auto [backend, processor] = makeFakeProcessor();
    backend->updateStatus = DataProcessorStatus::UnsupportedFeature;
    ufeel::Session session(std::move(processor));
    session.demand().pin(ufeel::DetectionType::HeartRate | ufeel::DetectionType::SpeechToText);

    session.start();

    ASSERT_TRUE(eventually([&session] -> bool {
        return !ufeel::hasAny(session.supportedFeatures(), ufeel::DetectionType::HeartRate);
    }));
    EXPECT_FALSE(ufeel::hasAny(session.supportedFeatures(), ufeel::DetectionType::SpeechToText));
    EXPECT_TRUE(ufeel::hasAll(session.supportedFeatures(), ufeel::imageDetectionTypes));
    EXPECT_TRUE(eventually([&session] -> bool {
        return session.latest()->heartRate.status == ufeel::HeartRateStatus::NoData;
    }));
}

TEST(
    SessionTest, ReportsDetectionFailures
)
{
    auto [backend, processor] = makeFakeProcessor();
    backend->updateStatus = DataProcessorStatus::InvalidArgument;
    ufeel::Session session(std::move(processor));
    session.demand().pin(ufeel::DetectionType::HeartRate);

    session.start();

    EXPECT_TRUE(eventually([&session] -> bool {
        return session.latest()->message ==
               "Detection failed (InvalidArgument), retrying on the next frame";
    }));
}

TEST(
    SessionTest, ExecutesCommandsOnTheSessionThread
)
{
    auto [backend, processor] = makeFakeProcessor();
    ufeel::Session session(std::move(processor));
    auto commandThread = session.execute([](ufeel::Processor&) -> std::thread::id {
        return std::this_thread::get_id();
    });
    auto failure = session.execute([](ufeel::Processor&) -> void {
        throw std::runtime_error("command failure");
    });

    session.start();

    EXPECT_NE(commandThread.get(), std::this_thread::get_id());
    EXPECT_THROW(failure.get(), std::runtime_error);
}

TEST(
    SessionTest, CancelsPendingCommandsWhenDestroyed
)
{
    std::future<int> pending;
    {
        auto [backend, processor] = makeFakeProcessor();
        ufeel::Session session(std::move(processor));
        pending = session.execute([](ufeel::Processor&) -> int { return 1; });
    }

    EXPECT_THROW(pending.get(), std::future_error);
}

TEST(
    SessionTest, RejectsASecondStart
)
{
    auto [backend, processor] = makeFakeProcessor();
    ufeel::Session session(std::move(processor));

    session.start();

    EXPECT_THROW(session.start(), std::logic_error);
}

TEST(
    SessionTest, TogglesSpeechToText
)
{
    auto [backend, processor] = makeFakeProcessor();
    ufeel::Session session(std::move(processor));
    session.start();

    session.setSpeechToTextEnabled(true).get();

    EXPECT_TRUE(session.isSpeechToTextEnabled());
    ASSERT_TRUE(eventually([&session] -> bool {
        return session.latest()->speechToText.isValid();
    }));
    EXPECT_EQ(session.latest()->speechToText.text, "recognized");

    session.setSpeechToTextEnabled(false).get();

    EXPECT_FALSE(session.isSpeechToTextEnabled());
    EXPECT_EQ(session.latest()->speechToText.status, ufeel::SpeechToTextStatus::Disabled);
}

TEST(
    SessionTest, ListsAndSelectsMicrophones
)
{
    auto [backend, processor] = makeFakeProcessor();
    ufeel::Session session(std::move(processor));
    session.start();

    EXPECT_EQ(session.microphones().get().size(), 2U);
    EXPECT_TRUE(session.selectMicrophone("bluez_input.headset").get());
    EXPECT_EQ(session.selectedMicrophoneId(), "bluez_input.headset");
    EXPECT_FALSE(session.selectMicrophone("unplugged").get());
    EXPECT_EQ(session.selectedMicrophoneId(), "bluez_input.headset");
    EXPECT_TRUE(session.selectMicrophone("").get());
    EXPECT_EQ(session.selectedMicrophoneId(), "");
}

TEST(
    SessionTest, NotifiesMicrophoneChanges
)
{
    auto [backend, processor] = makeFakeProcessor();
    backend->microphonesChanged = true;
    std::atomic<bool> notified = false;
    ufeel::Session session(
        std::move(processor),
        {
            .onMicrophonesChanged = [&notified] -> void { notified = true; },
        }
    );

    session.start();

    EXPECT_TRUE(eventually([&notified] -> bool { return notified.load(); }));
}
