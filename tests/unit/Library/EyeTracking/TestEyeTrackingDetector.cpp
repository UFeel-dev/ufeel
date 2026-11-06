#include "EyeTracking/EyeTrackingCalibrationModel.hpp"
#include "EyeTracking/EyeTrackingDetector.hpp"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"
#include "opencv2/imgcodecs.hpp"

#include <cstddef>
#include <gtest/gtest.h>
#include <limits>

namespace
{

auto createDetector() -> EyeTrackingDetector
{
    return EyeTrackingDetector(DATA_PROCESSOR_MODEL_DIR_PATH);
}

} // namespace

TEST(
    EyeTrackingDetectorTest, ReportsFrameAndDetectionAvailability
)
{
    auto detector = createDetector();
    const cv::Mat frame(480, 640, CV_8UC3, cv::Scalar(0, 0, 0));

    EXPECT_EQ(detector.process(cv::Mat{}).status, EyeTrackingStatus::FrameUnavailable);

    detector.toggleEyeTrackingDetection(false);
    EXPECT_EQ(detector.process(frame).status, EyeTrackingStatus::Disabled);

    detector.toggleEyeTrackingDetection(true);
    EXPECT_EQ(detector.process(frame).status, EyeTrackingStatus::FaceUnavailable);
    detector.close();
}

TEST(
    EyeTrackingDetectorTest, ProcessesAvailableFaceImage
)
{
    auto detector = createDetector();
    const auto frame =
        cv::imread(std::string(RESOURCES_DIR_PATH) + "/astronaut.png", cv::IMREAD_COLOR);
    ASSERT_FALSE(frame.empty());

    const auto result = detector.process(frame);
    EXPECT_NE(result.status, EyeTrackingStatus::FrameUnavailable);
    EXPECT_NE(result.status, EyeTrackingStatus::Disabled);
}

TEST(
    EyeTrackingDetectorTest, ValidatesCalibrationTargetsAndReportsFailure
)
{
    auto detector = createDetector();
    const float nan = std::numeric_limits<float>::quiet_NaN();

    EXPECT_FALSE(detector.setCalibrationTarget(0.5F, 0.5F));
    EXPECT_FALSE(detector.finishCalibration());
    EXPECT_EQ(
        detector.calibrationDiagnostics().failureReason, CalibrationFailureReason::NotCalibrating
    );

    detector.beginCalibration();
    EXPECT_FALSE(detector.setCalibrationTarget(nan, 0.5F));
    EXPECT_FALSE(detector.setCalibrationTarget(-0.1F, 0.5F));
    EXPECT_FALSE(detector.setCalibrationTarget(1.1F, 0.5F));
    EXPECT_FALSE(detector.setCalibrationTarget(0.5F, -0.1F));
    EXPECT_FALSE(detector.setCalibrationTarget(0.5F, 1.1F));
    EXPECT_TRUE(detector.setCalibrationTarget(0.5F, 0.5F));
    EXPECT_TRUE(detector.setCalibrationTarget(0.5F, 0.5F));
    EXPECT_TRUE(detector.setCalibrationTarget(0.2F, 0.8F));

    const auto progress = detector.calibrationProgress();
    EXPECT_EQ(progress.targetCount, 2U);
    EXPECT_EQ(progress.targetsWithEnoughSamples, 0U);
    EXPECT_EQ(progress.samplesForCurrentTarget, 0U);
    EXPECT_EQ(progress.requiredSamplesPerTarget, GAZE_MINIMUM_SAMPLES_PER_TARGET);

    EXPECT_FALSE(detector.finishCalibration());
    EXPECT_EQ(
        detector.calibrationDiagnostics().failureReason,
        CalibrationFailureReason::InsufficientSamples
    );
    EXPECT_FALSE(detector.saveCalibrationProfile(""));
    EXPECT_FALSE(detector.loadCalibrationProfile(""));

    detector.beginCalibration();
    detector.cancelCalibration();
    EXPECT_EQ(detector.calibrationProgress().targetCount, 0U);
    EXPECT_EQ(detector.calibrationDiagnostics().failureReason, CalibrationFailureReason::None);
}

TEST(
    EyeTrackingDetectorTest, EnforcesMaximumCalibrationTargetCount
)
{
    auto detector = createDetector();
    detector.beginCalibration();

    for (std::size_t index = 0; index < GAZE_MAXIMUM_CALIBRATION_TARGETS; ++index)
    {
        const auto x = static_cast<float>(index % 8) / 7.0F;
        const auto row = index / 8;
        const auto y = static_cast<float>(row) / 7.0F;
        ASSERT_TRUE(detector.setCalibrationTarget(x, y));
    }

    EXPECT_EQ(detector.calibrationProgress().targetCount, GAZE_MAXIMUM_CALIBRATION_TARGETS);
    EXPECT_FALSE(detector.setCalibrationTarget(0.37F, 0.41F));
}
