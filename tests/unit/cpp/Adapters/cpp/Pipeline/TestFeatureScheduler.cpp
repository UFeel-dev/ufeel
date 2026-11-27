#include "ufeel/Model/Detection.hpp"
#include "ufeel/Model/Flags.hpp"
#include "ufeel/Pipeline/Clock.hpp"
#include "ufeel/Pipeline/FeatureScheduler.hpp"

#include <chrono>
#include <gtest/gtest.h>
#include <stdexcept>

using ufeel::DetectionType;

namespace
{

constexpr ufeel::FeatureIntervals intervals{
    .emotions = std::chrono::milliseconds(200),
    .eyeTracking = std::chrono::milliseconds(50),
    .speechToText = std::chrono::milliseconds(100),
    .heartRate = std::chrono::seconds(1),
};

} // namespace

TEST(
    FeatureSchedulerTest, RunsAtMostOneImageFeaturePerTick
)
{
    ufeel::FeatureScheduler scheduler(intervals);
    const ufeel::Clock::time_point start{};

    EXPECT_EQ(
        scheduler.takeDue(start, ufeel::allDetectionTypes),
        DetectionType::Emotions | DetectionType::SpeechToText | DetectionType::HeartRate
    );
    EXPECT_EQ(scheduler.takeDue(start, ufeel::allDetectionTypes), DetectionType::EyeTracking);
    EXPECT_EQ(scheduler.takeDue(start, ufeel::allDetectionTypes), DetectionType::None);
}

TEST(
    FeatureSchedulerTest, RunsTheMostOverdueImageFeature
)
{
    ufeel::FeatureScheduler scheduler(intervals);
    const ufeel::Clock::time_point start{};
    static_cast<void>(scheduler.takeDue(start, ufeel::imageDetectionTypes));
    static_cast<void>(scheduler.takeDue(start, ufeel::imageDetectionTypes));

    const auto later = start + std::chrono::milliseconds(200);

    EXPECT_EQ(scheduler.takeDue(later, ufeel::imageDetectionTypes), DetectionType::EyeTracking);
    EXPECT_EQ(scheduler.takeDue(later, ufeel::imageDetectionTypes), DetectionType::Emotions);
}

TEST(
    FeatureSchedulerTest, SkipsInactiveFeatures
)
{
    ufeel::FeatureScheduler scheduler(intervals);

    EXPECT_EQ(
        scheduler.takeDue(ufeel::Clock::time_point{}, DetectionType::HeartRate),
        DetectionType::HeartRate
    );
}

TEST(
    FeatureSchedulerTest, AppliesAndClearsOverrides
)
{
    ufeel::FeatureScheduler scheduler(intervals);
    const ufeel::Clock::time_point start{};
    static_cast<void>(scheduler.takeDue(start, DetectionType::HeartRate));

    scheduler.setOverride(DetectionType::HeartRate, ufeel::Clock::duration::zero());
    EXPECT_EQ(scheduler.takeDue(start, DetectionType::HeartRate), DetectionType::HeartRate);

    scheduler.clearOverride(DetectionType::HeartRate);
    EXPECT_EQ(scheduler.takeDue(start, DetectionType::HeartRate), DetectionType::None);
}

TEST(
    FeatureSchedulerTest, RejectsOverridesOfSeveralFeatures
)
{
    ufeel::FeatureScheduler scheduler;

    EXPECT_THROW(
        scheduler.setOverride(ufeel::imageDetectionTypes, std::chrono::milliseconds(1)),
        std::invalid_argument
    );
}
