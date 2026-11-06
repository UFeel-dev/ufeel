#include "DataProcessor.hpp"

#include "Emotions/EmotionDetector.hpp"
#include "EyeTracking/EyeTrackingCalibrationModel.hpp"
#include "EyeTracking/EyeTrackingDetector.hpp"
#include "HeartRate/HeartRateDetector.hpp"
#include "HeartRate/HeartRateSource.hpp"
#include "SpeechToText/SpeechToTextDetector.hpp"

#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <opencv2/core/mat.hpp>
#include <opencv2/videoio.hpp>
#include <string>
#include <utility>

namespace
{

constexpr auto kValidDetectionMask =
    static_cast<std::uint32_t>(Emotions) | static_cast<std::uint32_t>(EyeTracking) |
    static_cast<std::uint32_t>(Speech) | static_cast<std::uint32_t>(HeartRate);

constexpr auto needsVideoFrame(
    std::uint32_t detectionType
) noexcept -> bool
{
    return hasDetection(detectionType, Emotions) || hasDetection(detectionType, EyeTracking);
}

auto makeEmotionResult(
    std::map<std::string, float> emotions
) -> EmotionResult
{
    return {
        .status = emotions.empty() ? EmotionStatus::FaceUnavailable : EmotionStatus::Valid,
        .emotions = std::move(emotions),
    };
}

} // namespace

DataProcessor::DataProcessor(
    const std::string& modelDirectory, const std::string& speechModelPath
)
    : cap_(0)
{
    if (!cap_.isOpened())
    {
        std::cerr << "[ERROR] Cannot open camera capture_id=0\n";
    }

    emotionDetector_ = std::make_unique<EmotionDetector>(modelDirectory);

    eyeTrackingDetector_ = std::make_unique<EyeTrackingDetector>(modelDirectory);

    speechToTextDetector_ = std::make_unique<SpeechToTextDetector>(speechModelPath);

    heartRateSensorDetector_ = std::make_unique<HeartRateSensorDetector>();
}

auto DataProcessor::updateFrame() -> DataProcessorStatus
{
    cv::Mat capturedFrame;
    if (!cap_.read(capturedFrame) || capturedFrame.empty())
    {
        frame_.release();
        return DataProcessorStatus::FrameUnavailable;
    }

    frame_ = std::move(capturedFrame);
    return DataProcessorStatus::Ok;
}

auto DataProcessor::setFrame(
    const cv::Mat& frame
) -> DataProcessorStatus
{
    if (frame.empty())
    {
        return DataProcessorStatus::InvalidFrame;
    }

    cv::Mat ownedFrame = frame.clone();
    if (ownedFrame.empty())
    {
        return DataProcessorStatus::InvalidFrame;
    }

    frame_ = std::move(ownedFrame);
    return DataProcessorStatus::Ok;
}

auto DataProcessor::getFrame() const -> cv::Mat
{
    return frame_;
}

auto DataProcessor::updateFromFrame(
    uint32_t detectionType, const cv::Mat& image
) -> DataProcessorStatus
{
    if ((detectionType & ~kValidDetectionMask) != 0)
    {
        return DataProcessorStatus::InvalidDetectionType;
    }

    if (needsVideoFrame(detectionType))
    {
        const auto frameStatus = setFrame(image);
        if (frameStatus != DataProcessorStatus::Ok)
        {
            if (hasDetection(detectionType, Emotions))
            {
                emotions_ = {.status = EmotionStatus::FrameUnavailable};
            }
            if (hasDetection(detectionType, EyeTracking))
            {
                eyeTracking_ = {.status = EyeTrackingStatus::FrameUnavailable};
            }
            return frameStatus;
        }
    }

    if (hasDetection(detectionType, Emotions))
    {
        emotions_ = makeEmotionResult(emotionDetector_->process(frame_));
    }

    if (hasDetection(detectionType, EyeTracking))
    {
        eyeTracking_ = eyeTrackingDetector_->process(frame_);
    }

    if (hasDetection(detectionType, Speech))
    {
        const auto text = speechToTextDetector_->process();
        speechToText_ = {
            .status = !speechToTextEnabled_ ? SpeechToTextStatus::Disabled
                                            : (text.empty() ? SpeechToTextStatus::NoSpeech
                                                            : SpeechToTextStatus::Valid),
            .text = speechToTextEnabled_ ? text : std::string{},
        };
    }

    if (hasDetection(detectionType, HeartRate))
    {
        heartRate_ = heartRateSensorDetector_->process();
    }

    return DataProcessorStatus::Ok;
}

auto DataProcessor::update(
    uint32_t detectionType
) -> DataProcessorStatus
{
    if ((detectionType & ~kValidDetectionMask) != 0)
    {
        return DataProcessorStatus::InvalidDetectionType;
    }

    if (needsVideoFrame(detectionType))
    {
        const auto frameStatus = updateFrame();
        if (frameStatus != DataProcessorStatus::Ok)
        {
            if (hasDetection(detectionType, Emotions))
            {
                emotions_ = {.status = EmotionStatus::FrameUnavailable};
            }
            if (hasDetection(detectionType, EyeTracking))
            {
                eyeTracking_ = {.status = EyeTrackingStatus::FrameUnavailable};
            }
            return frameStatus;
        }
    }

    return updateFromFrame(detectionType, frame_);
}

auto DataProcessor::updateEmotions() -> DataProcessorStatus
{
    const auto frameStatus = updateFrame();
    if (frameStatus != DataProcessorStatus::Ok)
    {
        emotions_ = {.status = EmotionStatus::FrameUnavailable};
        return frameStatus;
    }

    emotions_ = makeEmotionResult(emotionDetector_->process(frame_));
    return DataProcessorStatus::Ok;
}

auto DataProcessor::updateEmotionsFromFrame(
    const cv::Mat& frame
) -> DataProcessorStatus
{
    const auto frameStatus = setFrame(frame);
    if (frameStatus != DataProcessorStatus::Ok)
    {
        emotions_ = {.status = EmotionStatus::FrameUnavailable};
        return frameStatus;
    }

    emotions_ = makeEmotionResult(emotionDetector_->process(frame_));
    return DataProcessorStatus::Ok;
}

auto DataProcessor::getEmotions() const -> const EmotionResult&
{
    return emotions_;
}

auto DataProcessor::getEmotionsFromFrame(
    const cv::Mat& frame
) const -> EmotionResult
{
    if (frame.empty())
    {
        return {.status = EmotionStatus::FrameUnavailable};
    }

    return makeEmotionResult(emotionDetector_->process(frame));
}

auto DataProcessor::updateEyeTracking() -> DataProcessorStatus
{
    const auto frameStatus = updateFrame();
    if (frameStatus != DataProcessorStatus::Ok)
    {
        eyeTracking_ = {.status = EyeTrackingStatus::FrameUnavailable};
        return frameStatus;
    }

    eyeTracking_ = eyeTrackingDetector_->process(frame_);
    return DataProcessorStatus::Ok;
}

auto DataProcessor::updateEyeTrackingFromFrame(
    const cv::Mat& frame
) -> DataProcessorStatus
{
    const auto frameStatus = setFrame(frame);
    if (frameStatus != DataProcessorStatus::Ok)
    {
        eyeTracking_ = {.status = EyeTrackingStatus::FrameUnavailable};
        return frameStatus;
    }

    eyeTracking_ = eyeTrackingDetector_->process(frame_);
    return DataProcessorStatus::Ok;
}

auto DataProcessor::getEyeTracking() const -> EyeTrackingResult
{
    return eyeTracking_;
}

auto DataProcessor::getEyeTrackingFromFrame(
    const cv::Mat& frame
) const -> EyeTrackingResult
{
    if (frame.empty())
    {
        return {.status = EyeTrackingStatus::FrameUnavailable};
    }

    return eyeTrackingDetector_->process(frame);
}

auto DataProcessor::updateSpeechToText() -> DataProcessorStatus
{
    const auto text = speechToTextDetector_->process();
    speechToText_ = {
        .status = !speechToTextEnabled_
                      ? SpeechToTextStatus::Disabled
                      : (text.empty() ? SpeechToTextStatus::NoSpeech : SpeechToTextStatus::Valid),
        .text = speechToTextEnabled_ ? text : std::string{},
    };
    return DataProcessorStatus::Ok;
}

auto DataProcessor::getSpeechToText() const -> const SpeechToTextResult&
{
    return speechToText_;
}

void DataProcessor::beginEyeTrackingCalibration()
{
    eyeTrackingDetector_->beginCalibration();
}

auto DataProcessor::setEyeTrackingCalibrationTarget(
    float x, float y
) -> bool
{
    return eyeTrackingDetector_->setCalibrationTarget(x, y);
}

auto DataProcessor::finishEyeTrackingCalibration() -> bool
{
    return eyeTrackingDetector_->finishCalibration();
}

void DataProcessor::cancelEyeTrackingCalibration()
{
    eyeTrackingDetector_->cancelCalibration();
}

auto DataProcessor::getEyeTrackingCalibrationProgress() const -> CalibrationProgress
{
    return eyeTrackingDetector_->calibrationProgress();
}

auto DataProcessor::getEyeTrackingCalibrationDiagnostics() const -> CalibrationDiagnostics
{
    return eyeTrackingDetector_->calibrationDiagnostics();
}

auto DataProcessor::saveEyeTrackingCalibrationProfile(
    const std::string& path
) const -> bool
{
    return eyeTrackingDetector_->saveCalibrationProfile(path);
}

auto DataProcessor::loadEyeTrackingCalibrationProfile(
    const std::string& path
) -> bool
{
    return eyeTrackingDetector_->loadCalibrationProfile(path);
}

auto DataProcessor::updateHeartRate() -> DataProcessorStatus
{
    heartRate_ = heartRateSensorDetector_->process();
    return DataProcessorStatus::Ok;
}

auto DataProcessor::getHeartRate() const -> HeartRateResult
{
    return heartRate_;
}

void DataProcessor::toggleSpeechToText(
    bool state
)
{
    speechToTextEnabled_ = state;
    if (!state)
    {
        speechToText_ = {.status = SpeechToTextStatus::Disabled};
    }

    speechToTextDetector_->toggle(state);
}

DataProcessor::~DataProcessor()
{
    if (cap_.isOpened())
    {
        cap_.release();
    }
}
