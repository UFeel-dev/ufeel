#include "ufeel/Model/Detection.hpp"
#include "ufeel/Model/Flags.hpp"

#include <cstddef>
#include <gtest/gtest.h>
#include <stdexcept>

using ufeel::DetectionType;

TEST(
    DetectionTest, CombinesAndMasksFlags
)
{
    const DetectionType image = DetectionType::Emotions | DetectionType::EyeTracking;

    EXPECT_EQ(image, ufeel::imageDetectionTypes);
    EXPECT_EQ(
        ufeel::allDetectionTypes & ~image, DetectionType::SpeechToText | DetectionType::HeartRate
    );
    EXPECT_TRUE(ufeel::hasAny(image, DetectionType::EyeTracking | DetectionType::HeartRate));
    EXPECT_FALSE(ufeel::hasAll(image, DetectionType::EyeTracking | DetectionType::HeartRate));
    EXPECT_FALSE(ufeel::hasAny(DetectionType::None, ufeel::allDetectionTypes));
}

TEST(
    DetectionTest, UpdatesFlagsInPlace
)
{
    DetectionType features = DetectionType::Emotions;

    features |= DetectionType::HeartRate;
    EXPECT_EQ(features, DetectionType::Emotions | DetectionType::HeartRate);

    features &= DetectionType::HeartRate;
    EXPECT_EQ(features, DetectionType::HeartRate);
}

TEST(
    DetectionTest, IndexesEachSingleDetectionType
)
{
    for (std::size_t index = 0; index < ufeel::singleDetectionTypes.size(); ++index)
    {
        EXPECT_EQ(ufeel::detectionIndex(ufeel::singleDetectionTypes.at(index)), index);
    }
}

TEST(
    DetectionTest, RejectsCombinedOrEmptyDetectionTypes
)
{
    EXPECT_THROW(
        static_cast<void>(ufeel::detectionIndex(ufeel::imageDetectionTypes)), std::invalid_argument
    );
    EXPECT_THROW(
        static_cast<void>(ufeel::detectionIndex(DetectionType::None)), std::invalid_argument
    );
}
