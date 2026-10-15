#include "EyeTracking/EyeTrackingDetector.hpp"

#include "EyeTracking/GazeCalibrationModel.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <opencv2/core/saturate.hpp>
#include <optional>
#include <string>

namespace
{

constexpr double BLINK_EAR_THRESHOLD = 0.12;
constexpr double DIRECTION_DEAD_ZONE = 0.12;
constexpr std::size_t IRIS_LANDMARK_COUNT = 5;

using Vector3 = std::array<double, 3>;

[[nodiscard]] auto normalizedEyePosition(
    cv::Point corner1,
    cv::Point corner2,
    cv::Point upperLid,
    cv::Point lowerLid,
    const std::vector<cv::Point>& irisLandmarks
) -> std::optional<cv::Point2d>
{
    if (irisLandmarks.empty())
    {
        return std::nullopt;
    }

    auto const pupil = irisLandmarks[0];
    cv::Point2d const horizontal(
        static_cast<double>(corner2.x - corner1.x), static_cast<double>(corner2.y - corner1.y)
    );
    cv::Point2d const vertical(
        static_cast<double>(lowerLid.x - upperLid.x), static_cast<double>(lowerLid.y - upperLid.y)
    );
    double const width = cv::norm(horizontal);
    double const height = cv::norm(vertical);
    if (width < 2.0 || height < 2.0)
    {
        return std::nullopt;
    }

    cv::Point2d const horizontalAxis = horizontal * (1.0 / width);
    cv::Point2d verticalAxis = vertical * (1.0 / height);
    verticalAxis -= horizontalAxis * verticalAxis.dot(horizontalAxis);
    double const orthogonalHeight = cv::norm(verticalAxis);
    if (orthogonalHeight < 1.0e-6)
    {
        return std::nullopt;
    }
    verticalAxis *= 1.0 / orthogonalHeight;

    cv::Point2d const pupilOffset(
        static_cast<double>(pupil.x - corner1.x), static_cast<double>(pupil.y - corner1.y)
    );
    cv::Point2d const upperOffset(
        static_cast<double>(upperLid.x - corner1.x), static_cast<double>(upperLid.y - corner1.y)
    );
    double const relativeY =
        (pupilOffset - upperOffset).dot(verticalAxis) / (height * orthogonalHeight);
    cv::Point2d const result(pupilOffset.dot(horizontalAxis) / width, relativeY);
    if (!std::isfinite(result.x) || !std::isfinite(result.y))
    {
        return std::nullopt;
    }
    return result;
}

[[nodiscard]] auto eyeAspectRatio(
    cv::Point corner1,
    cv::Point corner2,
    cv::Point upper1,
    cv::Point lower1,
    cv::Point upper2,
    cv::Point lower2
) -> double
{
    double const width = cv::norm(corner2 - corner1);
    if (width < 2.0)
    {
        return 0.0;
    }
    double const firstHeight = cv::norm(upper1 - lower1);
    double const secondHeight = cv::norm(upper2 - lower2);
    return (firstHeight + secondHeight) / (2.0 * width);
}

[[nodiscard]] auto vectorFrom(
    const cv::Point3f& point
) -> Vector3
{
    return {point.x, point.y, point.z};
}

[[nodiscard]] auto subtract(
    const Vector3& left, const Vector3& right
) -> Vector3
{
    return {left[0] - right[0], left[1] - right[1], left[2] - right[2]};
}

[[nodiscard]] auto dot(
    const Vector3& left, const Vector3& right
) -> double
{
    return left[0] * right[0] + left[1] * right[1] + left[2] * right[2];
}

[[nodiscard]] auto cross(
    const Vector3& left, const Vector3& right
) -> Vector3
{
    return {
        left[1] * right[2] - left[2] * right[1],
        left[2] * right[0] - left[0] * right[2],
        left[0] * right[1] - left[1] * right[0]};
}

[[nodiscard]] auto normalized(
    Vector3 value
) -> std::optional<Vector3>
{
    double const length = std::sqrt(dot(value, value));
    if (!std::isfinite(length) || length < 1.0e-6)
    {
        return std::nullopt;
    }
    for (auto& coordinate : value)
    {
        coordinate /= length;
    }
    return value;
}

[[nodiscard]] auto finite(
    const Vector3& point
) -> bool
{
    return std::ranges::all_of(point, [](double value) { return std::isfinite(value); });
}

[[nodiscard]] auto qualityOf(
    const cv::Point2d& left,
    const cv::Point2d& right,
    double leftEar,
    double rightEar,
    double imageEyeSpan,
    double minimumFrameDimension
) -> float
{
    double const scaleQuality =
        std::clamp(imageEyeSpan / (minimumFrameDimension * 0.10), 0.35, 1.0);
    double const maximumIrisOffset = std::max(
        std::abs(left.x - 0.5),
        std::max(std::abs(right.x - 0.5), std::max(std::abs(left.y - 0.5), std::abs(right.y - 0.5)))
    );
    double const irisQuality = std::clamp(1.0 - maximumIrisOffset, 0.35, 1.0);
    double const opennessQuality = std::clamp(std::min(leftEar, rightEar) / 0.25, 0.45, 1.0);
    return static_cast<float>(0.45 * scaleQuality + 0.35 * irisQuality + 0.20 * opennessQuality);
}

} // namespace

EyeTrackingDetector::EyeTrackingDetector(
    const std::string& modelDirectory
)
    : irisLandmarker_(modelDirectory)
{
}

EyeTrackingDetector::~EyeTrackingDetector() = default;

void EyeTrackingDetector::toggleEyeTrackingDetection(
    bool state
)
{
    processEnable_ = state;
    if (!state)
    {
        smoothedGaze_.reset();
        lastGazeTime_.reset();
    }
    std::cout << "[EyeTrackingDetector] eye tracking detection " << (state ? "enabled" : "disabled")
              << '\n';
}

auto EyeTrackingDetector::process(
    const cv::Mat& frame
) -> GazeResult
{
    result_ = {};
    if (!processEnable_)
    {
        result_.status = GazeStatus::Disabled;
        return result_;
    }
    if (frame.empty())
    {
        result_.status = GazeStatus::FrameUnavailable;
        return result_;
    }

    GazeStatus featureStatus = GazeStatus::FaceUnavailable;
    float featureQuality = 0.0F;
    auto features = extractFeatures(frame, featureStatus, featureQuality);
    if (!features)
    {
        result_.status = featureStatus;
        return result_;
    }

    if (calibrationActive_)
    {
        if (calibrationTargetIndex_ && *calibrationTargetIndex_ < calibrationTargets_.size())
        {
            auto& samples = calibrationTargets_[*calibrationTargetIndex_].samples;
            if (samples.size() < GAZE_MAXIMUM_SAMPLES_PER_TARGET)
            {
                samples.push_back(*features);
            }
        }
        result_.status = GazeStatus::Calibrating;
        result_.confidence = featureQuality;
        return result_;
    }
    if (!calibrationModel_.isValid())
    {
        result_.status = GazeStatus::NotCalibrated;
        return result_;
    }

    auto const prediction = calibrationModel_.predict(*features);
    if (!prediction || prediction->x < -0.25F || prediction->x > 1.25F || prediction->y < -0.25F ||
        prediction->y > 1.25F)
    {
        result_.status = GazeStatus::LowQuality;
        return result_;
    }

    cv::Point2f const rawGaze(
        std::clamp(prediction->x, 0.0F, 1.0F), std::clamp(prediction->y, 0.0F, 1.0F)
    );
    auto const now = std::chrono::steady_clock::now();
    double const secondsSincePrevious =
        lastGazeTime_ ? std::chrono::duration<double>(now - *lastGazeTime_).count()
                      : std::numeric_limits<double>::infinity();
    if (!smoothedGaze_ || secondsSincePrevious > 0.5)
    {
        smoothedGaze_ = rawGaze;
    }
    else
    {
        double const timeConstant =
            0.10 / std::clamp(static_cast<double>(featureQuality), 0.35, 1.0);
        double const alpha = 1.0 - std::exp(-secondsSincePrevious / timeConstant);
        smoothedGaze_->x += static_cast<float>(alpha * (rawGaze.x - smoothedGaze_->x));
        smoothedGaze_->y += static_cast<float>(alpha * (rawGaze.y - smoothedGaze_->y));
    }
    lastGazeTime_ = now;

    result_.x = smoothedGaze_->x;
    result_.y = smoothedGaze_->y;
    result_.confidence = featureQuality * calibrationModel_.confidence();
    result_.status = GazeStatus::Valid;

    if (result_.x < 0.5 - DIRECTION_DEAD_ZONE)
    {
        result_.directions |= GazeLeft;
    }
    else if (result_.x > 0.5 + DIRECTION_DEAD_ZONE)
    {
        result_.directions |= GazeRight;
    }

    if (result_.y < 0.5 - DIRECTION_DEAD_ZONE)
    {
        result_.directions |= GazeUp;
    }
    else if (result_.y > 0.5 + DIRECTION_DEAD_ZONE)
    {
        result_.directions |= GazeDown;
    }

    if ((result_.directions & (GazeLeft | GazeRight | GazeUp | GazeDown)) == 0)
    {
        result_.directions |= GazeCenter;
    }
    return result_;
}

void EyeTrackingDetector::beginCalibration()
{
    calibrationTargets_.clear();
    calibrationTargetIndex_.reset();
    calibrationActive_ = true;
}

auto EyeTrackingDetector::setCalibrationTarget(
    float x, float y
) -> bool
{
    if (!calibrationActive_ || !std::isfinite(x) || !std::isfinite(y) || x < 0.0F || x > 1.0F ||
        y < 0.0F || y > 1.0F)
    {
        return false;
    }

    cv::Point2f const newTarget(x, y);
    auto target =
        std::ranges::find_if(calibrationTargets_, [&newTarget](GazeCalibrationTarget const& item) {
            return cv::norm(item.target - newTarget) < 0.02;
        });
    if (target == calibrationTargets_.end())
    {
        if (calibrationTargets_.size() >= GAZE_MAXIMUM_CALIBRATION_TARGETS)
        {
            return false;
        }
        calibrationTargets_.push_back({newTarget, {}});
        calibrationTargetIndex_ = calibrationTargets_.size() - 1;
    }
    else
    {
        calibrationTargetIndex_ =
            static_cast<std::size_t>(std::distance(calibrationTargets_.begin(), target));
    }
    return true;
}

auto EyeTrackingDetector::finishCalibration() -> bool
{
    if (!calibrationActive_)
    {
        return false;
    }

    calibrationActive_ = false;
    calibrationTargetIndex_.reset();
    auto fittedModel = GazeCalibrationModel::fit(calibrationTargets_);
    if (!fittedModel)
    {
        return false;
    }

    calibrationModel_ = *fittedModel;
    smoothedGaze_.reset();
    lastGazeTime_.reset();
    calibrationTargets_.clear();
    return true;
}

void EyeTrackingDetector::cancelCalibration()
{
    calibrationActive_ = false;
    calibrationTargetIndex_.reset();
    calibrationTargets_.clear();
}

auto EyeTrackingDetector::extractFeatures(
    const cv::Mat& frame, GazeStatus& status, float& quality
) -> std::optional<GazeFeatureVector>
{
    status = GazeStatus::FaceUnavailable;
    quality = 0.0F;
    irisLandmarker_.loadImageToInput(frame);
    irisLandmarker_.runInference();
    if (irisLandmarker_.getFaceRoi().empty())
    {
        return std::nullopt;
    }

    auto const leftIris = irisLandmarker_.getAllEyeLandmarks(true, true);
    auto const rightIris = irisLandmarker_.getAllEyeLandmarks(false, true);
    if (leftIris.size() != IRIS_LANDMARK_COUNT || rightIris.size() != IRIS_LANDMARK_COUNT ||
        irisLandmarker_.getEyeRoi(true).empty() || irisLandmarker_.getEyeRoi(false).empty())
    {
        status = GazeStatus::LowQuality;
        return std::nullopt;
    }

    auto const leftPosition = normalizedEyePosition(
        irisLandmarker_.getFaceLandmarkAt(362),
        irisLandmarker_.getFaceLandmarkAt(263),
        irisLandmarker_.getFaceLandmarkAt(386),
        irisLandmarker_.getFaceLandmarkAt(374),
        leftIris
    );
    auto const rightPosition = normalizedEyePosition(
        irisLandmarker_.getFaceLandmarkAt(33),
        irisLandmarker_.getFaceLandmarkAt(133),
        irisLandmarker_.getFaceLandmarkAt(159),
        irisLandmarker_.getFaceLandmarkAt(145),
        rightIris
    );
    if (!leftPosition || !rightPosition)
    {
        status = GazeStatus::LowQuality;
        return std::nullopt;
    }

    double const leftEar = eyeAspectRatio(
        irisLandmarker_.getFaceLandmarkAt(362),
        irisLandmarker_.getFaceLandmarkAt(263),
        irisLandmarker_.getFaceLandmarkAt(386),
        irisLandmarker_.getFaceLandmarkAt(374),
        irisLandmarker_.getFaceLandmarkAt(387),
        irisLandmarker_.getFaceLandmarkAt(373)
    );
    double const rightEar = eyeAspectRatio(
        irisLandmarker_.getFaceLandmarkAt(33),
        irisLandmarker_.getFaceLandmarkAt(133),
        irisLandmarker_.getFaceLandmarkAt(159),
        irisLandmarker_.getFaceLandmarkAt(145),
        irisLandmarker_.getFaceLandmarkAt(158),
        irisLandmarker_.getFaceLandmarkAt(153)
    );
    if (leftEar < BLINK_EAR_THRESHOLD && rightEar < BLINK_EAR_THRESHOLD)
    {
        status = GazeStatus::Blinking;
        return std::nullopt;
    }
    if (leftEar < BLINK_EAR_THRESHOLD || rightEar < BLINK_EAR_THRESHOLD || leftEar > 0.65 ||
        rightEar > 0.65)
    {
        status = GazeStatus::LowQuality;
        return std::nullopt;
    }

    auto const leftOuter = vectorFrom(irisLandmarker_.getFaceLandmark3DAt(33));
    auto const rightOuter = vectorFrom(irisLandmarker_.getFaceLandmark3DAt(263));
    auto const nose = vectorFrom(irisLandmarker_.getFaceLandmark3DAt(1));
    auto const chin = vectorFrom(irisLandmarker_.getFaceLandmark3DAt(152));
    if (!finite(leftOuter) || !finite(rightOuter) || !finite(nose) || !finite(chin))
    {
        status = GazeStatus::LowQuality;
        return std::nullopt;
    }

    Vector3 const eyeAxis = subtract(rightOuter, leftOuter);
    double const eyeSpan = std::sqrt(dot(eyeAxis, eyeAxis));
    auto const leftOuter2d = irisLandmarker_.getFaceLandmarkAt(33);
    auto const rightOuter2d = irisLandmarker_.getFaceLandmarkAt(263);
    double const imageEyeSpan = cv::norm(rightOuter2d - leftOuter2d);
    double const minimumFrameDimension = static_cast<double>(std::min(frame.cols, frame.rows));
    if (!std::isfinite(eyeSpan) || eyeSpan < 2.0 || imageEyeSpan < minimumFrameDimension * 0.025)
    {
        status = GazeStatus::LowQuality;
        return std::nullopt;
    }

    auto const xAxis = normalized(eyeAxis);
    Vector3 const eyeCenter = {
        (leftOuter[0] + rightOuter[0]) * 0.5,
        (leftOuter[1] + rightOuter[1]) * 0.5,
        (leftOuter[2] + rightOuter[2]) * 0.5};
    Vector3 vertical = subtract(chin, eyeCenter);
    if (!xAxis)
    {
        status = GazeStatus::LowQuality;
        return std::nullopt;
    }
    double const verticalProjection = dot(vertical, *xAxis);
    for (std::size_t axis = 0; axis < vertical.size(); ++axis)
    {
        vertical[axis] -= verticalProjection * (*xAxis)[axis];
    }
    auto const yAxis = normalized(vertical);
    if (!yAxis)
    {
        status = GazeStatus::LowQuality;
        return std::nullopt;
    }
    auto const zAxis = normalized(cross(*xAxis, *yAxis));
    if (!zAxis)
    {
        status = GazeStatus::LowQuality;
        return std::nullopt;
    }
    Vector3 const noseOffset = subtract(nose, eyeCenter);

    GazeFeatureVector features = {
        leftPosition->x,
        leftPosition->y,
        rightPosition->x,
        rightPosition->y,
        dot(noseOffset, *xAxis) / eyeSpan,
        dot(noseOffset, *yAxis) / eyeSpan,
        dot(noseOffset, *zAxis) / eyeSpan,
        (*xAxis)[2],
        (*xAxis)[1],
        (*yAxis)[2],
        leftEar,
        rightEar};
    if (!std::ranges::all_of(features, [](double value) { return std::isfinite(value); }) ||
        leftPosition->x < -0.25 || leftPosition->x > 1.25 || rightPosition->x < -0.25 ||
        rightPosition->x > 1.25 || leftPosition->y < -0.5 || leftPosition->y > 1.5 ||
        rightPosition->y < -0.5 || rightPosition->y > 1.5)
    {
        status = GazeStatus::LowQuality;
        return std::nullopt;
    }

    quality = qualityOf(
        *leftPosition, *rightPosition, leftEar, rightEar, imageEyeSpan, minimumFrameDimension
    );
    status = GazeStatus::Valid;
    return features;
}

auto EyeTrackingDetector::calibrationProgress() const -> CalibrationProgress
{
    CalibrationProgress progress;
    progress.targetCount = static_cast<std::uint32_t>(calibrationTargets_.size());
    progress.requiredSamplesPerTarget = static_cast<std::uint32_t>(GAZE_MINIMUM_SAMPLES_PER_TARGET);
    for (auto const& target : calibrationTargets_)
    {
        if (target.samples.size() >= GAZE_MINIMUM_SAMPLES_PER_TARGET)
        {
            ++progress.targetsWithEnoughSamples;
        }
    }
    if (calibrationTargetIndex_ && *calibrationTargetIndex_ < calibrationTargets_.size())
    {
        progress.samplesForCurrentTarget = static_cast<std::uint32_t>(
            calibrationTargets_[*calibrationTargetIndex_].samples.size()
        );
    }
    return progress;
}

auto EyeTrackingDetector::saveCalibrationProfile(
    const std::string& path
) const -> bool
{
    return calibrationModel_.save(path);
}

auto EyeTrackingDetector::loadCalibrationProfile(
    const std::string& path
) -> bool
{
    if (!calibrationModel_.load(path))
    {
        return false;
    }
    calibrationActive_ = false;
    calibrationTargets_.clear();
    calibrationTargetIndex_.reset();
    smoothedGaze_.reset();
    lastGazeTime_.reset();
    return true;
}

void EyeTrackingDetector::close() {}
