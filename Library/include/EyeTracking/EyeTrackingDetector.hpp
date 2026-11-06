#ifndef EYETRACKING_EYETRACKINGDETECTOR_HPP
#define EYETRACKING_EYETRACKINGDETECTOR_HPP

#include "EyeTracking/EyeTrackingCalibrationModel.hpp"
#include "EyeTracking/EyeTrackingKalmanFilter.hpp"
#include "EyeTracking/IrisDetection/IrisLandmark.hpp"
#include "Utils/FrameChecker.hpp"
#include "opencv2/core/types.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace cv
{

class Mat;

} // namespace cv

namespace ufeel::internal
{

class EyeTrackingLandmarkProvider
{
public:

    EyeTrackingLandmarkProvider() = default;
    virtual ~EyeTrackingLandmarkProvider() = default;
    EyeTrackingLandmarkProvider(const EyeTrackingLandmarkProvider&) = delete;
    auto operator=(const EyeTrackingLandmarkProvider&) -> EyeTrackingLandmarkProvider& = delete;
    EyeTrackingLandmarkProvider(EyeTrackingLandmarkProvider&&) = delete;
    auto operator=(EyeTrackingLandmarkProvider&&) -> EyeTrackingLandmarkProvider& = delete;

    virtual void loadImageToInput(const cv::Mat& frame) = 0;
    virtual void runInference() = 0;
    [[nodiscard]] virtual auto getFaceRoi() const -> cv::Rect = 0;
    [[nodiscard]] virtual auto getAllEyeLandmarks(bool leftEye, bool iris) const
        -> std::vector<cv::Point> = 0;
    [[nodiscard]] virtual auto getEyeRoi(bool leftEye) const -> cv::Rect = 0;
    [[nodiscard]] virtual auto getFaceLandmarkAt(int index) const -> cv::Point = 0;
    [[nodiscard]] virtual auto getFaceLandmark3DAt(int index) const -> cv::Point3f = 0;
};

} // namespace ufeel::internal

enum class EyeTrackingStatus : std::uint8_t
{
    NotCalibrated = 0,
    FaceUnavailable = 1,
    Blinking = 2,
    LowQuality = 3,
    Calibrating = 4,
    Valid = 5,
    Disabled = 6,
    FrameUnavailable = 7,
};

struct EyeTrackingResult
{
    float x = 0;
    float y = 0;
    float recordedX = 0;
    float recordedY = 0;
    float confidence = 0;
    uint32_t directions = 0;
    EyeTrackingStatus status = EyeTrackingStatus::NotCalibrated;
};

struct CalibrationProgress
{
    std::uint32_t targetsWithEnoughSamples = 0;
    std::uint32_t targetCount = 0;
    std::uint32_t samplesForCurrentTarget = 0;
    std::uint32_t requiredSamplesPerTarget = 0;
};

enum EyeTrackingDirectionFlags : std::uint8_t // NOLINT(cppcoreguidelines-use-enum-class): unscoped
                                              // bit flags are part of the existing public API.
{
    EyeTrackingLeft = 1U << 0U,
    EyeTrackingRight = 1U << 1U,
    EyeTrackingUp = 1U << 2U,
    EyeTrackingDown = 1U << 3U,
    EyeTrackingCenter = 1U << 4U,
};

class EyeTrackingDetector
{
public:

    explicit EyeTrackingDetector(const std::string& modelDirectory);
    explicit EyeTrackingDetector(
        std::unique_ptr<ufeel::internal::EyeTrackingLandmarkProvider> landmarkProvider
    );
    ~EyeTrackingDetector();

    EyeTrackingDetector(const EyeTrackingDetector&) = delete;
    auto operator=(const EyeTrackingDetector&) -> EyeTrackingDetector& = delete;
    EyeTrackingDetector(EyeTrackingDetector&&) = delete;
    auto operator=(EyeTrackingDetector&&) -> EyeTrackingDetector& = delete;

    auto process(const cv::Mat& frame) -> EyeTrackingResult;
    void toggleEyeTrackingDetection(bool state);
    void close();
    void beginCalibration();
    [[nodiscard]] auto setCalibrationTarget(float x, float y) -> bool;
    [[nodiscard]] auto finishCalibration() -> bool;
    void cancelCalibration();
    [[nodiscard]] auto calibrationProgress() const -> CalibrationProgress;
    [[nodiscard]] auto calibrationDiagnostics() const -> const CalibrationDiagnostics&;
    [[nodiscard]] auto saveCalibrationProfile(const std::string& path) const -> bool;
    [[nodiscard]] auto loadCalibrationProfile(const std::string& path) -> bool;

private:

    [[nodiscard]] auto
    extractFeatures(const cv::Mat& frame, EyeTrackingStatus& status, float& quality)
        -> std::optional<EyeTrackingFeatureVector>;

    bool processEnable_ = true;
    bool calibrationActive_ = false;
    std::optional<std::size_t> calibrationTargetIndex_;
    std::optional<std::chrono::steady_clock::time_point> calibrationTargetSetAt_;
    std::vector<EyeTrackingCalibrationTarget> calibrationTargets_;
    std::deque<double> eyeAspectHistory_;
    EyeTrackingCalibrationModel calibrationModel_;
    EyeTrackingKalmanFilter gazeEstimator_;
    EyeTrackingResult result_;

    CalibrationDiagnostics lastCalibrationDiagnostics_{};

    std::unique_ptr<ufeel::internal::EyeTrackingLandmarkProvider> landmarkProvider_;

    FrameChecker<EyeTrackingResult> frameChecker_;
};

// TODO: improve eye tracking to handle glasses and other bad inputs better

#endif // EYETRACKING_EYETRACKINGDETECTOR_HPP
