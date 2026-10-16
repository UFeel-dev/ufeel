#ifndef EYETRACKING_EYETRACKINGDETECTOR_HPP
#define EYETRACKING_EYETRACKINGDETECTOR_HPP

#include "EyeTracking/GazeCalibrationModel.hpp"
#include "EyeTracking/IrisDetection/IrisLandmark.hpp"
#include "opencv2/core/types.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <vector>

namespace cv
{

class Mat;

} // namespace cv

enum class GazeStatus : std::uint8_t
{
    NotCalibrated = 0,
    FaceUnavailable = 1,
    Blinking = 2,
    LowQuality = 3,
    Calibrating = 4,
    Valid = 5,
    Disabled = 6,
    FrameUnavailable = 7
};

struct GazeResult
{
        float x = 0.0F;
        float y = 0.0F;
        float confidence = 0.0F;
        std::uint32_t directions = 0;
        GazeStatus status = GazeStatus::NotCalibrated;
};

struct CalibrationProgress
{
        std::uint32_t targetsWithEnoughSamples = 0;
        std::uint32_t targetCount = 0;
        std::uint32_t samplesForCurrentTarget = 0;
        std::uint32_t requiredSamplesPerTarget = 0;
};

enum GazeDirectionFlags : std::uint8_t
{
    GazeLeft = 1U << 0U,
    GazeRight = 1U << 1U,
    GazeUp = 1U << 2U,
    GazeDown = 1U << 3U,
    GazeCenter = 1U << 4U
};

class EyeTrackingDetector
{
    public:

        explicit EyeTrackingDetector(const std::string& modelDirectory);
        ~EyeTrackingDetector();

        EyeTrackingDetector(const EyeTrackingDetector&) = delete;
        auto operator=(const EyeTrackingDetector&) -> EyeTrackingDetector& = delete;
        EyeTrackingDetector(EyeTrackingDetector&&) = delete;
        auto operator=(EyeTrackingDetector&&) -> EyeTrackingDetector& = delete;

        auto process(const cv::Mat& frame) -> GazeResult;
        void toggleEyeTrackingDetection(bool state);
        void close();
        void beginCalibration();
        [[nodiscard]] auto setCalibrationTarget(float x, float y) -> bool;
        [[nodiscard]] auto finishCalibration() -> bool;
        void cancelCalibration();
        [[nodiscard]] auto calibrationProgress() const -> CalibrationProgress;
        [[nodiscard]] auto saveCalibrationProfile(const std::string& path) const -> bool;
        [[nodiscard]] auto loadCalibrationProfile(const std::string& path) -> bool;

    private:

        [[nodiscard]] auto extractFeatures(const cv::Mat& frame, GazeStatus& status, float& quality)
            -> std::optional<GazeFeatureVector>;

        bool processEnable_ = true;
        bool calibrationActive_ = false;
        std::optional<std::size_t> calibrationTargetIndex_;
        std::optional<std::chrono::steady_clock::time_point> calibrationTargetSetAt_;
        std::vector<GazeCalibrationTarget> calibrationTargets_;
        std::deque<double> eyeAspectHistory_;
        GazeCalibrationModel calibrationModel_;
        std::optional<cv::Point2f> smoothedGaze_;
        std::optional<std::chrono::steady_clock::time_point> lastGazeTime_;
        GazeResult result_;

        my::IrisLandmark irisLandmarker_;
};

#endif // EYETRACKING_EYETRACKINGDETECTOR_HPP
