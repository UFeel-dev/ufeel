#include "EyeTracking/EyeTrackingCalibrationModel.hpp"
#include "EyeTracking/EyeTrackingDetector.hpp"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"
#include "opencv2/imgcodecs.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <vector>

#ifndef DATA_PROCESSOR_MODEL_DIR_PATH
#define DATA_PROCESSOR_MODEL_DIR_PATH ""
#endif // DATA_PROCESSOR_MODEL_DIR_PATH
#ifndef RESOURCES_DIR_PATH
#define RESOURCES_DIR_PATH ""
#endif // RESOURCES_DIR_PATH

namespace
{

auto createDetector() -> EyeTrackingDetector
{
    return EyeTrackingDetector(DATA_PROCESSOR_MODEL_DIR_PATH);
}

class LandmarkProviderFake final : public ufeel::internal::EyeTrackingLandmarkProvider
{
public:

    LandmarkProviderFake()
    {
        faceLandmarks.at(33) = {100, 100};
        faceLandmarks.at(133) = {140, 100};
        faceLandmarks.at(159) = {120, 104};
        faceLandmarks.at(145) = {120, 112};
        faceLandmarks.at(158) = {120, 104};
        faceLandmarks.at(153) = {120, 112};
        faceLandmarks.at(362) = {200, 100};
        faceLandmarks.at(263) = {240, 100};
        faceLandmarks.at(386) = {220, 104};
        faceLandmarks.at(374) = {220, 112};
        faceLandmarks.at(387) = {220, 104};
        faceLandmarks.at(373) = {220, 112};
        faceLandmarks3d.at(33) = {0.0F, 0.0F, 0.0F};
        faceLandmarks3d.at(263) = {40.0F, 0.0F, 0.0F};
        faceLandmarks3d.at(10) = {20.0F, -20.0F, 0.0F};
        leftIris.at(0) = {220, 108};
        rightIris.at(0) = {120, 108};
    }

    void loadImageToInput(
        const cv::Mat& frame
    ) override
    {
        static_cast<void>(frame);
    }

    void runInference() override {}

    [[nodiscard]] auto getFaceRoi() const -> cv::Rect override
    {
        return hasFace ? cv::Rect(0, 0, 640, 480) : cv::Rect{};
    }

    [[nodiscard]] auto getAllEyeLandmarks(
        bool leftEye, bool iris
    ) const -> std::vector<cv::Point> override
    {
        static_cast<void>(iris);
        auto const& landmarks = leftEye ? leftIris : rightIris;
        auto const count = leftEye ? leftIrisCount : rightIrisCount;
        return {landmarks.begin(), landmarks.begin() + static_cast<std::ptrdiff_t>(count)};
    }

    [[nodiscard]] auto getEyeRoi(
        bool leftEye
    ) const -> cv::Rect override
    {
        bool const available = leftEye ? leftEyeAvailable : rightEyeAvailable;
        if (available)
        {
            return {0, 0, 20, 10};
        }
        return {};
    }

    [[nodiscard]] auto getFaceLandmarkAt(
        int index
    ) const -> cv::Point override
    {
        return faceLandmarks.at(static_cast<std::size_t>(index));
    }

    [[nodiscard]] auto getFaceLandmark3DAt(
        int index
    ) const -> cv::Point3f override
    {
        return faceLandmarks3d.at(static_cast<std::size_t>(index));
    }

    bool hasFace = true;
    bool leftEyeAvailable = true;
    bool rightEyeAvailable = true;
    std::size_t leftIrisCount = 5;
    std::size_t rightIrisCount = 5;
    std::array<cv::Point, 5> leftIris{};
    std::array<cv::Point, 5> rightIris{};
    std::array<cv::Point, 468> faceLandmarks{};
    std::array<cv::Point3f, 468> faceLandmarks3d{};
};

auto makeTestFrame(
    int value = 0
) -> cv::Mat
{
    return {480, 640, CV_8UC3, cv::Scalar(value, value, value)};
}

auto makeCalibrationTargets() -> std::vector<EyeTrackingCalibrationTarget>
{
    constexpr std::array<float, 3> coordinates{0.1F, 0.5F, 0.9F};
    std::vector<EyeTrackingCalibrationTarget> targets;
    for (const auto y : coordinates)
    {
        for (const auto x : coordinates)
        {
            EyeTrackingCalibrationTarget target{.target = {x, y}};
            for (std::size_t sampleIndex = 0; sampleIndex < GAZE_MINIMUM_SAMPLES_PER_TARGET + 1;
                 ++sampleIndex)
            {
                EyeTrackingFeatureVector features{};
                auto const variation = static_cast<double>(sampleIndex) * 0.0001;
                features.at(0) = static_cast<double>(x) + variation;
                features.at(1) = static_cast<double>(y) - variation;
                target.samples.push_back(features);
            }
            targets.push_back(std::move(target));
        }
    }
    return targets;
}

class TemporaryProfile
{
public:

    TemporaryProfile()
        : path(std::filesystem::temp_directory_path() / "ufeel-eye-tracking-detector-test.dat")
    {
    }

    ~TemporaryProfile()
    {
        std::error_code error;
        std::filesystem::remove(path, error);
        std::filesystem::remove(path.string() + ".tmp", error);
        std::filesystem::remove(path.string() + ".bak", error);
    }

    TemporaryProfile(const TemporaryProfile&) = delete;
    auto operator=(const TemporaryProfile&) -> TemporaryProfile& = delete;
    TemporaryProfile(TemporaryProfile&&) = delete;
    auto operator=(TemporaryProfile&&) -> TemporaryProfile& = delete;

    std::filesystem::path path;
};

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

TEST(
    EyeTrackingDetectorTest, ExtractsFeaturesAndBoundsBlinkHistory
)
{
    auto detector = EyeTrackingDetector(std::make_unique<LandmarkProviderFake>());
    detector.beginCalibration();

    for (int index = 0; index <= 50; ++index)
    {
        EXPECT_EQ(detector.process(makeTestFrame(index)).status, EyeTrackingStatus::Calibrating);
    }
}

TEST(
    EyeTrackingDetectorTest, ReportsFeatureQualityFailures
)
{
    auto provider = std::make_unique<LandmarkProviderFake>();
    auto* providerState = provider.get();
    auto detector = EyeTrackingDetector(std::move(provider));

    providerState->hasFace = false;
    EXPECT_EQ(detector.process(makeTestFrame()).status, EyeTrackingStatus::FaceUnavailable);
    providerState->hasFace = true;

    providerState->leftIrisCount = 4;
    EXPECT_EQ(detector.process(makeTestFrame(1)).status, EyeTrackingStatus::LowQuality);
    providerState->leftIrisCount = 5;
    providerState->leftEyeAvailable = false;
    EXPECT_EQ(detector.process(makeTestFrame(2)).status, EyeTrackingStatus::LowQuality);
    providerState->leftEyeAvailable = true;

    providerState->faceLandmarks.at(263).x = 201;
    EXPECT_EQ(detector.process(makeTestFrame(3)).status, EyeTrackingStatus::LowQuality);
    providerState->faceLandmarks.at(263).x = 240;

    providerState->faceLandmarks.at(386).y = 105;
    providerState->faceLandmarks.at(374).y = 107;
    providerState->faceLandmarks.at(387).y = 105;
    providerState->faceLandmarks.at(373).y = 107;
    providerState->leftIris.at(0).y = 106;
    providerState->rightIris.at(0).y = 106;
    EXPECT_EQ(detector.process(makeTestFrame(4)).status, EyeTrackingStatus::Blinking);

    providerState->faceLandmarks.at(386).y = 80;
    providerState->faceLandmarks.at(374).y = 120;
    providerState->faceLandmarks.at(387).y = 80;
    providerState->faceLandmarks.at(373).y = 120;
    EXPECT_EQ(detector.process(makeTestFrame(5)).status, EyeTrackingStatus::LowQuality);

    providerState->faceLandmarks.at(386).y = 104;
    providerState->faceLandmarks.at(374).y = 112;
    providerState->faceLandmarks.at(387).y = 104;
    providerState->faceLandmarks.at(373).y = 112;
    providerState->faceLandmarks3d.at(33).x = std::numeric_limits<float>::infinity();
    EXPECT_EQ(detector.process(makeTestFrame(6)).status, EyeTrackingStatus::LowQuality);
    providerState->faceLandmarks3d.at(33).x = 0.0F;

    providerState->faceLandmarks.at(386) = {220, 104};
    providerState->faceLandmarks.at(374) = {222, 104};
    EXPECT_EQ(detector.process(makeTestFrame(7)).status, EyeTrackingStatus::LowQuality);
    providerState->faceLandmarks.at(374) = {220, 112};

    providerState->faceLandmarks3d.at(263).x = 1.0F;
    EXPECT_EQ(detector.process(makeTestFrame(8)).status, EyeTrackingStatus::LowQuality);
    providerState->faceLandmarks3d.at(263).x = 40.0F;

    providerState->faceLandmarks3d.at(10) = {20.0F, 0.0F, 0.0F};
    EXPECT_EQ(detector.process(makeTestFrame(9)).status, EyeTrackingStatus::LowQuality);
}

TEST(
    EyeTrackingDetectorTest, RejectsNullLandmarkProvider
)
{
    EXPECT_THROW(
        EyeTrackingDetector(std::unique_ptr<ufeel::internal::EyeTrackingLandmarkProvider>{}),
        std::invalid_argument
    );
}

TEST(
    EyeTrackingDetectorTest, AppliesLoadedCalibrationAndReportsGazeDirections
)
{
    const TemporaryProfile profile;
    auto model = EyeTrackingCalibrationModel::fit(makeCalibrationTargets());
    if (!model.has_value())
    {
        FAIL() << "synthetic calibration targets did not produce a model";
        return;
    }
    ASSERT_TRUE(model.value().save(profile.path.string()));

    auto provider = std::make_unique<LandmarkProviderFake>();
    auto* providerState = provider.get();
    auto detector = EyeTrackingDetector(std::move(provider));

    detector.beginCalibration();
    ASSERT_TRUE(detector.setCalibrationTarget(0.5F, 0.5F));
    ASSERT_TRUE(detector.loadCalibrationProfile(profile.path.string()));
    EXPECT_EQ(detector.calibrationProgress().targetCount, 0U);

    const auto centered = detector.process(makeTestFrame(1));
    EXPECT_EQ(centered.status, EyeTrackingStatus::Valid);
    EXPECT_NE(centered.directions & EyeTrackingCenter, 0U);

    providerState->leftIris.at(0) = {236, 111};
    const auto rightDown = detector.process(makeTestFrame(2));
    EXPECT_EQ(rightDown.status, EyeTrackingStatus::Valid);
    EXPECT_NE(rightDown.directions & EyeTrackingRight, 0U);
    EXPECT_NE(rightDown.directions & EyeTrackingDown, 0U);

    const auto cached = detector.process(makeTestFrame(2));
    EXPECT_FLOAT_EQ(cached.x, rightDown.x);
    EXPECT_FLOAT_EQ(cached.y, rightDown.y);

    auto leftProvider = std::make_unique<LandmarkProviderFake>();
    auto* leftProviderState = leftProvider.get();
    leftProviderState->leftIris.at(0) = {204, 105};
    auto leftDetector = EyeTrackingDetector(std::move(leftProvider));
    ASSERT_TRUE(leftDetector.loadCalibrationProfile(profile.path.string()));
    const auto leftUp = leftDetector.process(makeTestFrame(3));
    EXPECT_EQ(leftUp.status, EyeTrackingStatus::Valid);
    EXPECT_NE(leftUp.directions & EyeTrackingLeft, 0U);
    EXPECT_NE(leftUp.directions & EyeTrackingUp, 0U);
}
