#include "EyeTracking/EyeTrackingDetector.hpp"

#include "EyeTracking/EyeGazeDirection.hpp"
#include "EyeTracking/EyeTrackingCalibrationModel.hpp"
#include "EyeTracking/IrisDetection/IrisLandmark.hpp"
#include "Utils/FrameChecker.hpp"
#include "Utils/FrameFormat.hpp"
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
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{

constexpr auto CALIBRATION_SETTLE_TIME = std::chrono::milliseconds(1000);
constexpr double MAXIMUM_EYE_ASPECT_RATIO = 0.65;
constexpr double HORIZONTAL_GAZE_SIGN = -1.0;
constexpr std::size_t IRIS_LANDMARK_COUNT = 5;
constexpr std::size_t EYE_CONTOUR_LANDMARK_COUNT = 71;
constexpr std::size_t CONTOUR_CORNER_1 = 0;
constexpr std::size_t CONTOUR_CORNER_2 = 8;
constexpr std::array<std::pair<std::size_t, std::size_t>, 3> CONTOUR_LID_PAIRS{
    {{11, 3}, {12, 4}, {13, 5}}};
constexpr std::size_t UPPER_LID_CENTER_FIRST = 10;
constexpr std::size_t UPPER_LID_CENTER_LAST = 14;
constexpr float GAZE_MINIMUM_OPENNESS = 0.8F;
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

class IrisLandmarkProvider final : public ufeel::internal::EyeTrackingLandmarkProvider
{
public:

    explicit IrisLandmarkProvider(
        const std::string& modelDirectory
    )
        : landmarker_(modelDirectory)
    {
    }

    void loadImageToInput(
        const cv::Mat& frame
    ) override
    {
        landmarker_.loadImageToInput(frame);
    }

    void runInference() override
    {
        landmarker_.runInference();
    }

    [[nodiscard]] auto getFaceRoi() const -> cv::Rect override
    {
        return landmarker_.getFaceRoi();
    }

    [[nodiscard]] auto getAllEyeLandmarks(
        bool leftEye, bool iris
    ) const -> std::vector<cv::Point2f> override
    {
        return landmarker_.getAllEyeLandmarks(leftEye, iris);
    }

    [[nodiscard]] auto getEyeRoi(
        bool leftEye
    ) const -> cv::Rect override
    {
        return landmarker_.getEyeRoi(leftEye);
    }

    [[nodiscard]] auto getFaceLandmarkAt(
        int index
    ) const -> cv::Point2f override
    {
        auto const landmark = landmarker_.getFaceLandmark3DAt(index);
        return {landmark.x, landmark.y};
    }

    [[nodiscard]] auto getFaceLandmark3DAt(
        int index
    ) const -> cv::Point3f override
    {
        return landmarker_.getFaceLandmark3DAt(index);
    }

private:

    my::IrisLandmark landmarker_;
};

[[nodiscard]] auto normalizedEyePosition(
    cv::Point2f corner1,
    cv::Point2f corner2,
    cv::Point2f upperLid,
    cv::Point2f lowerLid,
    const std::vector<cv::Point2f>& irisLandmarks
) -> std::optional<cv::Point2d>
{
    if (irisLandmarks.empty())
    {
        return std::nullopt;
    }

    auto const pupil = irisLandmarks.at(0);

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

[[nodiscard]] auto contourOffset(
    cv::Point2f corner1,
    cv::Point2f corner2,
    const std::vector<cv::Point2f>& contour,
    cv::Point2f point
) -> std::optional<cv::Point2d>
{
    cv::Point2d const axis(
        static_cast<double>(corner2.x - corner1.x), static_cast<double>(corner2.y - corner1.y)
    );
    double const axisLength = cv::norm(axis);
    auto const contourCorner1 = contour.at(CONTOUR_CORNER_1);
    auto const contourCorner2 = contour.at(CONTOUR_CORNER_2);
    double const width = cv::norm(contourCorner2 - contourCorner1);
    if (!std::isfinite(axisLength) || !std::isfinite(width) || axisLength < 2.0 || width < 2.0)
    {
        return std::nullopt;
    }
    cv::Point2d const along = axis * (1.0 / axisLength);
    cv::Point2d const across(-along.y, along.x);
    cv::Point2d const offset(
        static_cast<double>(point.x) - ((contourCorner1.x + contourCorner2.x) * 0.5),
        static_cast<double>(point.y) - ((contourCorner1.y + contourCorner2.y) * 0.5)
    );
    cv::Point2d const result(offset.dot(along) / width, offset.dot(across) / width);
    if (!std::isfinite(result.x) || !std::isfinite(result.y))
    {
        return std::nullopt;
    }
    return result;
}

[[nodiscard]] auto upperLidCenter(
    const std::vector<cv::Point2f>& contour
) -> cv::Point2f
{
    cv::Point2f sum;
    for (std::size_t index = UPPER_LID_CENTER_FIRST; index <= UPPER_LID_CENTER_LAST; ++index)
    {
        sum += contour.at(index);
    }
    return sum * (1.0F / static_cast<float>(UPPER_LID_CENTER_LAST - UPPER_LID_CENTER_FIRST + 1));
}

[[nodiscard]] auto contourAspectRatio(
    const std::vector<cv::Point2f>& contour
) -> double
{
    double opening = 0.0;
    for (auto const& [upper, lower] : CONTOUR_LID_PAIRS)
    {
        opening += cv::norm(contour.at(upper) - contour.at(lower));
    }
    return opening / (static_cast<double>(CONTOUR_LID_PAIRS.size()) *
                      cv::norm(contour.at(CONTOUR_CORNER_2) - contour.at(CONTOUR_CORNER_1)));
}

[[nodiscard]] auto directionFlags(
    int horizontal, int vertical
) -> std::uint32_t
{
    std::uint32_t flags = 0;
    if (horizontal < 0)
    {
        flags |= EyeTrackingLeft;
    }
    else if (horizontal > 0)
    {
        flags |= EyeTrackingRight;
    }
    if (vertical < 0)
    {
        flags |= EyeTrackingUp;
    }
    else if (vertical > 0)
    {
        flags |= EyeTrackingDown;
    }
    return flags == 0 ? EyeTrackingCenter : flags;
}

[[nodiscard]] auto eyeAspectRatio(
    cv::Point2f corner1,
    cv::Point2f corner2,
    cv::Point2f upper1,
    cv::Point2f lower1,
    cv::Point2f upper2,
    cv::Point2f lower2
) -> double
{
    double const width = cv::norm(corner2 - corner1);
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
    const Vector3& minuend, const Vector3& subtrahend
) -> Vector3
{
    return {
        minuend.at(0) - subtrahend.at(0),
        minuend.at(1) - subtrahend.at(1),
        minuend.at(2) - subtrahend.at(2),
    };
}

[[nodiscard]] auto dot(
    const Vector3& left, const Vector3& right
) -> double
{
    return (left.at(0) * right.at(0)) + (left.at(1) * right.at(1)) + (left.at(2) * right.at(2));
}

[[nodiscard]] auto cross(
    const Vector3& left, const Vector3& right
) -> Vector3
{
    return {
        (left.at(1) * right.at(2)) - (left.at(2) * right.at(1)),
        (left.at(2) * right.at(0)) - (left.at(0) * right.at(2)),
        (left.at(0) * right.at(1)) - (left.at(1) * right.at(0)),
    };
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
    double const maximumIrisOffset = std::max({
        std::abs(left.x - 0.5),
        std::abs(right.x - 0.5),
        std::abs(left.y - 0.5),
        std::abs(right.y - 0.5),
    });
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
    : EyeTrackingDetector(std::make_unique<IrisLandmarkProvider>(modelDirectory + "/EyeTracking"))
{
}

EyeTrackingDetector::EyeTrackingDetector(
    std::unique_ptr<ufeel::internal::EyeTrackingLandmarkProvider> landmarkProvider
)
    : landmarkProvider_(std::move(landmarkProvider))
{
    if (!landmarkProvider_)
    {
        throw std::invalid_argument("eye tracking landmark provider must not be null");
    }
}

EyeTrackingDetector::~EyeTrackingDetector() = default;

void EyeTrackingDetector::toggleEyeTrackingDetection(
    bool state
)
{
    processEnable_ = state;
    if (!state)
    {
        resetTracking();
    }
    std::cout << "[EyeTrackingDetector] eye tracking detection " << (state ? "enabled" : "disabled")
              << '\n';
}

void EyeTrackingDetector::setHeadWeight(
    float weight
)
{
    if (std::isfinite(weight))
    {
        gazeDirection_.setHeadWeight(weight);
    }
}

auto EyeTrackingDetector::process(
    const cv::Mat& frame, Clock::time_point now
) -> EyeTrackingResult
{
    if (frameChecker_.isFrameIdentical(frame))
    {
        return frameChecker_.getResult();
    }

    result_ = processFrame(frame, now);
    if (result_.status == EyeTrackingStatus::Valid)
    {
        frameChecker_.setFrame(frame);
        frameChecker_.setResult(result_);
    }
    return result_;
}

auto EyeTrackingDetector::processFrame(
    const cv::Mat& frame, Clock::time_point now
) -> EyeTrackingResult
{
    EyeTrackingResult result;
    if (!processEnable_)
    {
        result.status = EyeTrackingStatus::Disabled;
        return result;
    }
    if (frame.empty())
    {
        result.status = EyeTrackingStatus::FrameUnavailable;
        return result;
    }

    EyeObservation observation;
    EyeTrackingStatus featureStatus = EyeTrackingStatus::FaceUnavailable;
    float featureQuality = 0.0F;
    auto const features = extractFeatures(frame, observation, featureStatus, featureQuality);

    if (!observation.aspectRatios)
    {
        resetTracking();
        result.status = featureStatus;
        result.blinkCount = blinkTracker_.state().blinkCount;
        return result;
    }

    auto const blink = blinkTracker_.update(
        observation.aspectRatios->first, observation.aspectRatios->second, now
    );
    auto const applyEyes = [&blink](EyeTrackingResult& target) -> void {
        target.leftOpenness = blink.leftOpenness;
        target.rightOpenness = blink.rightOpenness;
        target.eyeState = EyeStateTracked | (blink.leftClosed ? EyeStateLeftClosed : 0U) |
                          (blink.rightClosed ? EyeStateRightClosed : 0U);
        target.blinkCount = blink.blinkCount;
        target.eyesClosedSeconds = blink.eyesClosedSeconds;
    };

    if (blink.leftClosed || blink.rightClosed ||
        std::min(blink.leftOpenness, blink.rightOpenness) < GAZE_MINIMUM_OPENNESS)
    {
        result = heldResult_.value_or(EyeTrackingResult{.status = statusWithoutPosition()});
        applyEyes(result);
        return result;
    }

    if (observation.gaze)
    {
        auto const gaze = gazeDirection_.update(*observation.gaze, now);
        result.gazeX = gaze.x;
        result.gazeY = gaze.y;
        result.directions = directionFlags(gaze.horizontal, gaze.vertical);
    }
    else
    {
        gazeDirection_.resetMotion();
    }
    applyEyes(result);

    auto const hold = [this, &result]() -> EyeTrackingResult {
        heldResult_ = result;
        return result;
    };

    if (!features)
    {
        gazeEstimator_.reset();
        result.status = featureStatus;
        return hold();
    }

    if (calibrationActive_)
    {
        if (calibrationTargetIndex_ && *calibrationTargetIndex_ < calibrationTargets_.size())
        {
            auto& samples = calibrationTargets_.at(*calibrationTargetIndex_).samples;
            if (calibrationTargetSetAt_ &&
                now - *calibrationTargetSetAt_ >= CALIBRATION_SETTLE_TIME &&
                samples.size() < GAZE_MAXIMUM_SAMPLES_PER_TARGET)
            {
                samples.push_back(*features);
                if (observation.gaze)
                {
                    calibrationGazeSamples_.at(*calibrationTargetIndex_)
                        .push_back(*observation.gaze);
                }
            }
        }
        result.status = EyeTrackingStatus::Calibrating;
        result.confidence = featureQuality;
        return hold();
    }
    if (!calibrationModel_.isValid())
    {
        result.status = EyeTrackingStatus::NotCalibrated;
        return hold();
    }

    auto const prediction = calibrationModel_.predict(*features);
    if (!prediction || prediction->x < -0.25F || prediction->x > 1.25F || prediction->y < -0.25F ||
        prediction->y > 1.25F)
    {
        result.status = EyeTrackingStatus::LowQuality;
        return hold();
    }

    cv::Point2f const rawEyeTracking(
        std::clamp(prediction->x, 0.0F, 1.0F), std::clamp(prediction->y, 0.0F, 1.0F)
    );
    cv::Point2f const estimatedEyeTracking =
        gazeEstimator_.update(rawEyeTracking, featureQuality, now);

    result.recordedX = rawEyeTracking.x;
    result.recordedY = rawEyeTracking.y;
    result.x = estimatedEyeTracking.x;
    result.y = estimatedEyeTracking.y;
    result.confidence = featureQuality * calibrationModel_.confidence();
    result.status = EyeTrackingStatus::Valid;
    return hold();
}

auto EyeTrackingDetector::statusWithoutPosition() const -> EyeTrackingStatus
{
    if (calibrationActive_)
    {
        return EyeTrackingStatus::Calibrating;
    }
    return calibrationModel_.isValid() ? EyeTrackingStatus::LowQuality
                                       : EyeTrackingStatus::NotCalibrated;
}

void EyeTrackingDetector::resetTracking()
{
    gazeEstimator_.reset();
    gazeDirection_.resetMotion();
    blinkTracker_.interrupt();
    heldResult_.reset();
}

void EyeTrackingDetector::beginCalibration()
{
    calibrationTargets_.clear();
    calibrationGazeSamples_.clear();
    calibrationTargetIndex_.reset();
    calibrationTargetSetAt_.reset();
    gazeEstimator_.reset();
    heldResult_.reset();
    calibrationActive_ = true;
    lastCalibrationDiagnostics_ = {};
}

auto EyeTrackingDetector::setCalibrationTarget(
    float x, float y, Clock::time_point now
) -> bool
{
    if (!calibrationActive_ || !std::isfinite(x) || !std::isfinite(y) || x < 0.0F || x > 1.0F ||
        y < 0.0F || y > 1.0F)
    {
        return false;
    }

    cv::Point2f const newTarget(x, y);
    auto const previousTargetIndex = calibrationTargetIndex_;
    auto target = std::ranges::find_if(
        calibrationTargets_, [&newTarget](EyeTrackingCalibrationTarget const& item) -> bool {
            return cv::norm(item.target - newTarget) < 1.0e-4;
        }
    );
    std::size_t targetIndex = 0;
    if (target == calibrationTargets_.end())
    {
        if (calibrationTargets_.size() >= GAZE_MAXIMUM_CALIBRATION_TARGETS)
        {
            return false;
        }
        calibrationTargets_.push_back({.target = newTarget, .samples = {}});
        calibrationGazeSamples_.emplace_back();
        targetIndex = calibrationTargets_.size() - 1;
    }
    else
    {
        targetIndex = static_cast<std::size_t>(std::distance(calibrationTargets_.begin(), target));
    }
    calibrationTargetIndex_ = targetIndex;
    if (!previousTargetIndex || *previousTargetIndex != targetIndex)
    {
        calibrationTargetSetAt_ = now;
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

    auto fittedModel =
        EyeTrackingCalibrationModel::fit(calibrationTargets_, &lastCalibrationDiagnostics_);

    std::vector<std::pair<cv::Point2f, std::vector<EyeGazeSample>>> gazeTargets;
    gazeTargets.reserve(calibrationTargets_.size());
    for (std::size_t index = 0; index < calibrationTargets_.size(); ++index)
    {
        gazeTargets.emplace_back(
            calibrationTargets_.at(index).target, std::move(calibrationGazeSamples_.at(index))
        );
    }
    calibrationTargets_.clear();
    calibrationGazeSamples_.clear();
    gazeEstimator_.reset();
    heldResult_.reset();

    if (!fittedModel)
    {
        return false;
    }

    fittedModel->setGazeRange(EyeGazeDirection::fitRange(gazeTargets));
    calibrationModel_ = *fittedModel;
    gazeDirection_.applyRange(calibrationModel_.gazeRange());
    return true;
}

void EyeTrackingDetector::cancelCalibration()
{
    calibrationActive_ = false;
    calibrationTargetIndex_.reset();
    calibrationTargetSetAt_.reset();
    calibrationTargets_.clear();
    calibrationGazeSamples_.clear();
    gazeEstimator_.reset();
    heldResult_.reset();
    lastCalibrationDiagnostics_ = {};
}

auto EyeTrackingDetector::extractFeatures(
    const cv::Mat& frame, EyeObservation& observation, EyeTrackingStatus& status, float& quality
) -> std::optional<EyeTrackingFeatureVector>
{
    status = EyeTrackingStatus::FaceUnavailable;
    quality = 0.0F;
    landmarkProvider_->loadImageToInput(toBgr(frame));
    landmarkProvider_->runInference();
    if (landmarkProvider_->getFaceRoi().empty())
    {
        return std::nullopt;
    }

    auto const leftIris = landmarkProvider_->getAllEyeLandmarks(true, true);
    auto const rightIris = landmarkProvider_->getAllEyeLandmarks(false, true);
    auto const leftContour = landmarkProvider_->getAllEyeLandmarks(true, false);
    auto const rightContour = landmarkProvider_->getAllEyeLandmarks(false, false);
    if (leftIris.size() != IRIS_LANDMARK_COUNT || rightIris.size() != IRIS_LANDMARK_COUNT ||
        leftContour.size() != EYE_CONTOUR_LANDMARK_COUNT ||
        rightContour.size() != EYE_CONTOUR_LANDMARK_COUNT ||
        landmarkProvider_->getEyeRoi(true).empty() || landmarkProvider_->getEyeRoi(false).empty())
    {
        status = EyeTrackingStatus::LowQuality;
        return std::nullopt;
    }

    double const leftEar = eyeAspectRatio(
        landmarkProvider_->getFaceLandmarkAt(362),
        landmarkProvider_->getFaceLandmarkAt(263),
        landmarkProvider_->getFaceLandmarkAt(386),
        landmarkProvider_->getFaceLandmarkAt(374),
        landmarkProvider_->getFaceLandmarkAt(387),
        landmarkProvider_->getFaceLandmarkAt(373)
    );
    double const rightEar = eyeAspectRatio(
        landmarkProvider_->getFaceLandmarkAt(33),
        landmarkProvider_->getFaceLandmarkAt(133),
        landmarkProvider_->getFaceLandmarkAt(159),
        landmarkProvider_->getFaceLandmarkAt(145),
        landmarkProvider_->getFaceLandmarkAt(158),
        landmarkProvider_->getFaceLandmarkAt(153)
    );
    auto const plausibleEar = [](double ear) -> bool {
        return std::isfinite(ear) && ear > 0.0 && ear <= MAXIMUM_EYE_ASPECT_RATIO;
    };
    if (!plausibleEar(leftEar) || !plausibleEar(rightEar))
    {
        status = EyeTrackingStatus::LowQuality;
        return std::nullopt;
    }
    double const leftOpening = contourAspectRatio(leftContour);
    double const rightOpening = contourAspectRatio(rightContour);
    if (!plausibleEar(leftOpening) || !plausibleEar(rightOpening))
    {
        status = EyeTrackingStatus::LowQuality;
        return std::nullopt;
    }
    observation.aspectRatios = std::pair{leftOpening, rightOpening};

    auto const leftPosition = normalizedEyePosition(
        landmarkProvider_->getFaceLandmarkAt(362),
        landmarkProvider_->getFaceLandmarkAt(263),
        landmarkProvider_->getFaceLandmarkAt(386),
        landmarkProvider_->getFaceLandmarkAt(374),
        leftIris
    );
    auto const rightPosition = normalizedEyePosition(
        landmarkProvider_->getFaceLandmarkAt(33),
        landmarkProvider_->getFaceLandmarkAt(133),
        landmarkProvider_->getFaceLandmarkAt(159),
        landmarkProvider_->getFaceLandmarkAt(145),
        rightIris
    );
    if (!leftPosition || !rightPosition)
    {
        status = EyeTrackingStatus::LowQuality;
        return std::nullopt;
    }

    auto const leftOuter = vectorFrom(landmarkProvider_->getFaceLandmark3DAt(33));
    auto const rightOuter = vectorFrom(landmarkProvider_->getFaceLandmark3DAt(263));
    auto const topOfHead = vectorFrom(landmarkProvider_->getFaceLandmark3DAt(10));
    if (!finite(leftOuter) || !finite(rightOuter) || !finite(topOfHead))
    {
        status = EyeTrackingStatus::LowQuality;
        return std::nullopt;
    }

    Vector3 const eyeAxis = subtract(rightOuter, leftOuter);
    double const eyeSpan = std::sqrt(dot(eyeAxis, eyeAxis));
    auto const leftOuter2d = landmarkProvider_->getFaceLandmarkAt(33);
    auto const rightOuter2d = landmarkProvider_->getFaceLandmarkAt(263);
    double const imageEyeSpan = cv::norm(rightOuter2d - leftOuter2d);
    auto const minimumFrameDimension = static_cast<double>(std::min(frame.cols, frame.rows));
    if (!std::isfinite(eyeSpan) || eyeSpan < 2.0 || imageEyeSpan < minimumFrameDimension * 0.025)
    {
        status = EyeTrackingStatus::LowQuality;
        return std::nullopt;
    }

    auto const xAxis = normalized(eyeAxis);
    Vector3 const eyeCenter = {
        (leftOuter.at(0) + rightOuter.at(0)) * 0.5,
        (leftOuter.at(1) + rightOuter.at(1)) * 0.5,
        (leftOuter.at(2) + rightOuter.at(2)) * 0.5,
    };
    Vector3 vertical = subtract(topOfHead, eyeCenter);
    if (!xAxis)
    {
        status = EyeTrackingStatus::LowQuality;
        return std::nullopt;
    }
    double const verticalProjection = dot(vertical, *xAxis);
    for (std::size_t axis = 0; axis < vertical.size(); ++axis)
    {
        vertical.at(axis) -= verticalProjection * (*xAxis).at(axis);
    }
    auto const yAxis = normalized(vertical);
    if (!yAxis)
    {
        status = EyeTrackingStatus::LowQuality;
        return std::nullopt;
    }
    auto const zAxis = normalized(cross(*xAxis, *yAxis));
    if (!zAxis)
    {
        status = EyeTrackingStatus::LowQuality;
        return std::nullopt;
    }

    double const headRoll = std::atan2((*xAxis).at(1), (*xAxis).at(0));
    double const headYaw = std::atan2(-(*xAxis).at(2), std::hypot((*yAxis).at(2), (*zAxis).at(2)));
    double const headPitch = std::atan2((*yAxis).at(2), -(*zAxis).at(2));

    auto const leftCorner1 = landmarkProvider_->getFaceLandmarkAt(362);
    auto const leftCorner2 = landmarkProvider_->getFaceLandmarkAt(263);
    auto const rightCorner1 = landmarkProvider_->getFaceLandmarkAt(33);
    auto const rightCorner2 = landmarkProvider_->getFaceLandmarkAt(133);
    auto const leftIrisOffset =
        contourOffset(leftCorner1, leftCorner2, leftContour, leftIris.at(0));
    auto const rightIrisOffset =
        contourOffset(rightCorner1, rightCorner2, rightContour, rightIris.at(0));
    auto const leftLidOffset =
        contourOffset(leftCorner1, leftCorner2, leftContour, upperLidCenter(leftContour));
    auto const rightLidOffset =
        contourOffset(rightCorner1, rightCorner2, rightContour, upperLidCenter(rightContour));
    if (leftIrisOffset && rightIrisOffset && leftLidOffset && rightLidOffset &&
        std::isfinite(headYaw) && std::isfinite(headPitch))
    {
        observation.gaze = EyeGazeSample{
            .eyeX = HORIZONTAL_GAZE_SIGN * (leftIrisOffset->x + rightIrisOffset->x) * 0.5,
            .eyeY = (leftLidOffset->y + rightLidOffset->y) * 0.5,
            .headX = -HORIZONTAL_GAZE_SIGN * headYaw,
            .headY = -headPitch,
        };
    }

    EyeTrackingFeatureVector features{};
    std::size_t featureIndex = 0;
    auto appendFeature = [&features, &featureIndex](double value) -> bool {
        if (featureIndex >= features.size() || !std::isfinite(value))
        {
            return false;
        }
        features.at(featureIndex++) = value;
        return true;
    };
    if (!appendFeature(leftPosition->x) || !appendFeature(leftPosition->y) ||
        !appendFeature(rightPosition->x) || !appendFeature(rightPosition->y))
    {
        status = EyeTrackingStatus::LowQuality;
        return std::nullopt;
    }

    auto appendLandmarkFeatures = [&](int landmarkIndex) -> bool {
        auto const landmark = vectorFrom(landmarkProvider_->getFaceLandmark3DAt(landmarkIndex));
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
            status = EyeTrackingStatus::LowQuality;
            return std::nullopt;
        }
    }
    for (int const landmarkIndex : FACE_FEATURE_LANDMARKS)
    {
        if (!appendLandmarkFeatures(landmarkIndex))
        {
            status = EyeTrackingStatus::LowQuality;
            return std::nullopt;
        }
    }

    if (!appendFeature(headRoll) || !appendFeature(headYaw) || !appendFeature(headPitch) ||
        !appendFeature(leftEar) || !appendFeature(rightEar))
    {
        status = EyeTrackingStatus::LowQuality;
        return std::nullopt;
    }

    if (featureIndex != features.size() ||
        !std::ranges::all_of(features, [](double value) -> bool { return std::isfinite(value); }) ||
        leftPosition->x < -0.25 || leftPosition->x > 1.25 || rightPosition->x < -0.25 ||
        rightPosition->x > 1.25 || leftPosition->y < -0.5 || leftPosition->y > 1.5 ||
        rightPosition->y < -0.5 || rightPosition->y > 1.5)
    {
        status = EyeTrackingStatus::LowQuality;
        return std::nullopt;
    }

    quality = qualityOf(
        *leftPosition, *rightPosition, leftEar, rightEar, imageEyeSpan, minimumFrameDimension
    );
    status = EyeTrackingStatus::Valid;
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
            calibrationTargets_.at(*calibrationTargetIndex_).samples.size()
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
    EyeTrackingCalibrationModel loadedModel;

    if (!loadedModel.load(path))
    {
        return false;
    }

    calibrationModel_ = loadedModel;
    calibrationActive_ = false;
    calibrationTargets_.clear();
    calibrationGazeSamples_.clear();
    calibrationTargetIndex_.reset();
    calibrationTargetSetAt_.reset();
    gazeEstimator_.reset();
    heldResult_.reset();
    gazeDirection_.applyRange(calibrationModel_.gazeRange());
    lastCalibrationDiagnostics_ = {};

    return true;
}

void EyeTrackingDetector::close() {}
