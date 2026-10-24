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
#include <iterator>
#include <numeric>
#include <optional>
#include <string>
#include <vector>

namespace
{

constexpr std::size_t BLINK_EAR_HISTORY_LENGTH = 50;
constexpr std::size_t BLINK_EAR_MINIMUM_HISTORY = 15;
constexpr double BLINK_EAR_THRESHOLD_RATIO = 0.8;
constexpr double INITIAL_BLINK_EAR_THRESHOLD = 0.2;
constexpr auto CALIBRATION_SETTLE_TIME = std::chrono::milliseconds(1000);
constexpr double DIRECTION_DEAD_ZONE = 0.12;
constexpr std::size_t IRIS_LANDMARK_COUNT = 5;
constexpr int FACE_LANDMARK_COUNT = 468;
constexpr std::array<int, 8> EYE_FRAME_LANDMARKS = {33, 133, 159, 145, 263, 362, 386, 374};
constexpr std::array<int, 9> FACE_FEATURE_LANDMARKS = {4, 10, 151, 9, 152, 234, 454, 58, 288};
static_assert(
    GAZE_FEATURE_COUNT ==
    4 + (EYE_FRAME_LANDMARKS.size() * 3) + (FACE_FEATURE_LANDMARKS.size() * 3) + 5
);
static_assert(std::ranges::all_of(EYE_FRAME_LANDMARKS, [](int index) -> bool {
    return index >= 0 && index < FACE_LANDMARK_COUNT;
}));
static_assert(std::ranges::all_of(FACE_FEATURE_LANDMARKS, [](int index) -> bool {
    return index >= 0 && index < FACE_LANDMARK_COUNT;
}));

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

    if (!std::isfinite(width) || !std::isfinite(height) || width < 2.0 || height < 2.0)
    {
        return std::nullopt;
    }

    cv::Point2d const horizontalAxis = horizontal * (1.0 / width);
    cv::Point2d verticalAxis = vertical * (1.0 / height);

    verticalAxis -= horizontalAxis * verticalAxis.dot(horizontalAxis);

    double const orthogonalHeight = cv::norm(verticalAxis);
    if (!std::isfinite(orthogonalHeight) || orthogonalHeight < 1.0e-6)
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

    double const relativeX = pupilOffset.dot(horizontalAxis) / width;
    double const relativeY = (pupilOffset - upperOffset).dot(verticalAxis) / height;

    cv::Point2d const result(relativeX, relativeY);

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
    return (left[0] * right[0]) + (left[1] * right[1]) + (left[2] * right[2]);
}

[[nodiscard]] auto cross(
    const Vector3& left, const Vector3& right
) -> Vector3
{
    return {
        (left[1] * right[2]) - (left[2] * right[1]),
        (left[2] * right[0]) - (left[0] * right[2]),
        (left[0] * right[1]) - (left[1] * right[0])};
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
    return std::ranges::all_of(point, [](double value) -> bool { return std::isfinite(value); });
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
        {std::abs(left.x - 0.5),
        std::abs(right.x - 0.5), std::abs(left.y - 0.5), std::abs(right.y - 0.5)}
    );
    double const irisQuality = std::clamp(1.0 - maximumIrisOffset, 0.35, 1.0);
    double const opennessQuality = std::clamp(std::min(leftEar, rightEar) / 0.25, 0.45, 1.0);
    return static_cast<float>(
        (0.45 * scaleQuality) + (0.35 * irisQuality) + (0.20 * opennessQuality)
    );
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
        gazeEstimator_.reset();
        eyeAspectHistory_.clear();
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
        gazeEstimator_.reset();

        if (featureStatus == GazeStatus::FaceUnavailable ||
            featureStatus == GazeStatus::FrameUnavailable)
        {
            eyeAspectHistory_.clear();
        }

        result_.status = featureStatus;
        return result_;
    }

    if (calibrationActive_)
    {
        if (calibrationTargetIndex_ && *calibrationTargetIndex_ < calibrationTargets_.size())
        {
            auto& samples = calibrationTargets_[*calibrationTargetIndex_].samples;
            auto const now = std::chrono::steady_clock::now();
            if (calibrationTargetSetAt_ &&
                now - *calibrationTargetSetAt_ >= CALIBRATION_SETTLE_TIME &&
                samples.size() < GAZE_MAXIMUM_SAMPLES_PER_TARGET)
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

    cv::Point2f const estimatedGaze = gazeEstimator_.update(rawGaze, featureQuality);

    result_.recordedX = rawGaze.x;
    result_.recordedY = rawGaze.y;
    result_.x = estimatedGaze.x;
    result_.y = estimatedGaze.y;
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
    calibrationTargetSetAt_.reset();
    eyeAspectHistory_.clear();
    gazeEstimator_.reset();
    calibrationActive_ = true;
    lastCalibrationDiagnostics_ = {};
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
    auto const previousTargetIndex = calibrationTargetIndex_;
    auto target =
        std::ranges::find_if(calibrationTargets_, [&newTarget](GazeCalibrationTarget const& item) -> bool {
            return cv::norm(item.target - newTarget) < 1.0e-4;
        });
    std::size_t targetIndex = 0;
    if (target == calibrationTargets_.end())
    {
        if (calibrationTargets_.size() >= GAZE_MAXIMUM_CALIBRATION_TARGETS)
        {
            return false;
        }
        calibrationTargets_.push_back({.target=newTarget, .samples={}});
        targetIndex = calibrationTargets_.size() - 1;
    }
    else
    {
        targetIndex = static_cast<std::size_t>(std::distance(calibrationTargets_.begin(), target));
    }
    calibrationTargetIndex_ = targetIndex;
    if (!previousTargetIndex || *previousTargetIndex != targetIndex)
    {
        calibrationTargetSetAt_ = std::chrono::steady_clock::now();
    }
    return true;
}

auto EyeTrackingDetector::finishCalibration() -> bool
{
    if (!calibrationActive_)
    {
        lastCalibrationDiagnostics_ = {};
        lastCalibrationDiagnostics_.failureReason = CalibrationFailureReason::NotCalibrating;
        return false;
    }

    calibrationActive_ = false;
    calibrationTargetIndex_.reset();
    calibrationTargetSetAt_.reset();

    auto fittedModel = GazeCalibrationModel::fit(calibrationTargets_, &lastCalibrationDiagnostics_);

    calibrationTargets_.clear();
    gazeEstimator_.reset();

    if (!fittedModel)
    {
        return false;
    }

    calibrationModel_ = *fittedModel;
    return true;
}

void EyeTrackingDetector::cancelCalibration()
{
    calibrationActive_ = false;
    calibrationTargetIndex_.reset();
    calibrationTargetSetAt_.reset();
    calibrationTargets_.clear();
    eyeAspectHistory_.clear();
    gazeEstimator_.reset();
    lastCalibrationDiagnostics_ = {};
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
    double const averageEar = (leftEar + rightEar) * 0.5;
    double blinkThreshold = INITIAL_BLINK_EAR_THRESHOLD;
    if (eyeAspectHistory_.size() >= BLINK_EAR_MINIMUM_HISTORY)
    {
        double const meanEar =
            std::accumulate(eyeAspectHistory_.begin(), eyeAspectHistory_.end(), 0.0) /
            static_cast<double>(eyeAspectHistory_.size());
        blinkThreshold = meanEar * BLINK_EAR_THRESHOLD_RATIO;
    }
    if (averageEar < blinkThreshold || leftEar < blinkThreshold || rightEar < blinkThreshold)
    {
        status = GazeStatus::Blinking;
        return std::nullopt;
    }
    if (leftEar > 0.65 || rightEar > 0.65)
    {
        status = GazeStatus::LowQuality;
        return std::nullopt;
    }

    auto addEyeAspectSample = [&]() -> void {
        eyeAspectHistory_.push_back(averageEar);
        if (eyeAspectHistory_.size() > BLINK_EAR_HISTORY_LENGTH)
        {
            eyeAspectHistory_.pop_front();
        }
    };

    auto const leftOuter = vectorFrom(irisLandmarker_.getFaceLandmark3DAt(33));
    auto const rightOuter = vectorFrom(irisLandmarker_.getFaceLandmark3DAt(263));
    auto const topOfHead = vectorFrom(irisLandmarker_.getFaceLandmark3DAt(10));
    if (!finite(leftOuter) || !finite(rightOuter) || !finite(topOfHead))
    {
        status = GazeStatus::LowQuality;
        return std::nullopt;
    }

    Vector3 const eyeAxis =
        subtract(rightOuter, leftOuter); // ! we might be swapping left and right here, TODO: check
    double const eyeSpan = std::sqrt(dot(eyeAxis, eyeAxis));
    auto const leftOuter2d = irisLandmarker_.getFaceLandmarkAt(33);
    auto const rightOuter2d = irisLandmarker_.getFaceLandmarkAt(263);
    double const imageEyeSpan = cv::norm(rightOuter2d - leftOuter2d);
    auto const minimumFrameDimension = static_cast<double>(std::min(frame.cols, frame.rows));
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
    Vector3 vertical = subtract(topOfHead, eyeCenter);
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
    GazeFeatureVector features{};
    std::size_t featureIndex = 0;
    auto appendFeature = [&features, &featureIndex](double value) -> bool {
        if (featureIndex >= features.size() || !std::isfinite(value))
        {
            return false;
        }
        features[featureIndex++] = value;
        return true;
    };
    if (!appendFeature(leftPosition->x) || !appendFeature(leftPosition->y) ||
        !appendFeature(rightPosition->x) || !appendFeature(rightPosition->y))
    {
        status = GazeStatus::LowQuality;
        return std::nullopt;
    }

    auto appendLandmarkFeatures = [&](int landmarkIndex) -> bool {
        auto const landmark = vectorFrom(irisLandmarker_.getFaceLandmark3DAt(landmarkIndex));
        if (!finite(landmark))
        {
            return false;
        }
        Vector3 const offset = subtract(landmark, eyeCenter);
        return appendFeature(dot(offset, *xAxis) / eyeSpan) &&
               appendFeature(dot(offset, *yAxis) / eyeSpan) &&
               appendFeature(dot(offset, *zAxis) / eyeSpan);
    };

    for (int const landmarkIndex : EYE_FRAME_LANDMARKS)
    {
        if (!appendLandmarkFeatures(landmarkIndex))
        {
            status = GazeStatus::LowQuality;
            return std::nullopt;
        }
    }
    for (int const landmarkIndex : FACE_FEATURE_LANDMARKS)
    {
        if (!appendLandmarkFeatures(landmarkIndex))
        {
            status = GazeStatus::LowQuality;
            return std::nullopt;
        }
    }

    if (!appendFeature(std::atan2((*xAxis)[1], (*xAxis)[0])) ||
        !appendFeature(std::atan2(-(*xAxis)[2], std::hypot((*yAxis)[2], (*zAxis)[2]))) ||
        !appendFeature(std::atan2((*yAxis)[2], (*zAxis)[2])) || !appendFeature(leftEar) ||
        !appendFeature(rightEar))
    {
        status = GazeStatus::LowQuality;
        return std::nullopt;
    }

    if (featureIndex != features.size() ||
        !std::ranges::all_of(features, [](double value) -> bool { return std::isfinite(value); }) ||
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
    addEyeAspectSample();
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

auto EyeTrackingDetector::calibrationDiagnostics() const -> const CalibrationDiagnostics&
{
    return lastCalibrationDiagnostics_;
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
    GazeCalibrationModel loadedModel;

    if (!loadedModel.load(path))
    {
        return false;
    }

    calibrationModel_ = loadedModel;
    calibrationActive_ = false;
    calibrationTargets_.clear();
    calibrationTargetIndex_.reset();
    calibrationTargetSetAt_.reset();
    eyeAspectHistory_.clear();
    gazeEstimator_.reset();
    lastCalibrationDiagnostics_ = {};

    return true;
}

void EyeTrackingDetector::close() {}
