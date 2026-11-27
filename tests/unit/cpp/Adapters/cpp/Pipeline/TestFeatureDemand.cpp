#include "ufeel/Model/Detection.hpp"
#include "ufeel/Model/Flags.hpp"
#include "ufeel/Pipeline/Clock.hpp"
#include "ufeel/Pipeline/FeatureDemand.hpp"

#include <chrono>
#include <gtest/gtest.h>

using ufeel::DetectionType;

TEST(
    FeatureDemandTest, KeepsRequestedFeaturesActiveUntilTheTimeout
)
{
    ufeel::FeatureDemand demand(std::chrono::seconds(2));
    const ufeel::Clock::time_point start{};

    demand.request(DetectionType::Emotions | DetectionType::HeartRate, start);

    EXPECT_EQ(demand.active(start), DetectionType::Emotions | DetectionType::HeartRate);
    EXPECT_EQ(
        demand.active(start + std::chrono::seconds(2)),
        DetectionType::Emotions | DetectionType::HeartRate
    );
    EXPECT_EQ(demand.active(start + std::chrono::seconds(3)), DetectionType::None);
}

TEST(
    FeatureDemandTest, CountsPins
)
{
    ufeel::FeatureDemand demand;

    demand.pin(DetectionType::EyeTracking);
    demand.pin(DetectionType::EyeTracking | DetectionType::SpeechToText);
    demand.unpin(DetectionType::EyeTracking);

    EXPECT_EQ(demand.active(), DetectionType::EyeTracking | DetectionType::SpeechToText);

    demand.unpin(DetectionType::EyeTracking | DetectionType::SpeechToText);
    EXPECT_EQ(demand.active(), DetectionType::None);
}
