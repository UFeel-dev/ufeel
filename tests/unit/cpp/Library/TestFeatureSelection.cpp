#include "DataProcessor.hpp"
#include "opencv2/core/mat.hpp"

#include <functional>
#include <gtest/gtest.h>

namespace
{

auto createProcessor() -> DataProcessor
{
    return {"", "", [](cv::Mat&) -> bool { return false; }};
}

} // namespace

TEST(
    FeatureSelectionTest, DisabledFeaturesReportUnsupported
)
{
#if UFEEL_ENABLE_EMOTIONS && UFEEL_ENABLE_EYE_TRACKING && UFEEL_ENABLE_SPEECH_TO_TEXT &&           \
    UFEEL_ENABLE_HEART_RATE
    SUCCEED();
    return;
#else
    auto processor = createProcessor();
#if !UFEEL_ENABLE_EMOTIONS
    EXPECT_EQ(processor.update(Emotions), DataProcessorStatus::UnsupportedFeature);
#endif
#if !UFEEL_ENABLE_EYE_TRACKING
    EXPECT_EQ(processor.update(EyeTracking), DataProcessorStatus::UnsupportedFeature);
#endif
#if !UFEEL_ENABLE_SPEECH_TO_TEXT
    EXPECT_EQ(processor.update(Speech), DataProcessorStatus::UnsupportedFeature);
#endif
#if !UFEEL_ENABLE_HEART_RATE
    EXPECT_EQ(processor.update(HeartRate), DataProcessorStatus::UnsupportedFeature);
#endif
#endif
}
