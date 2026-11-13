#include "DataProcessor.hpp"

#include "Emotions/EmotionDetector.hpp"
#include "SpeechToText/SpeechToTextDetector.hpp"
#if UFEEL_ENABLE_EYE_TRACKING
#include "EyeTracking/EyeTrackingCalibrationModel.hpp"
#include "EyeTracking/EyeTrackingDetector.hpp"
#endif
#if UFEEL_ENABLE_HEART_RATE
#include "HeartRate/HeartRateDetector.hpp"
#include "HeartRate/HeartRateSource.hpp"
#endif

#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <opencv2/core/mat.hpp>
#include <opencv2/videoio.hpp>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{

constexpr auto kValidDetectionMask =
    static_cast<std::uint32_t>(Emotions) | static_cast<std::uint32_t>(EyeTracking) |
    static_cast<std::uint32_t>(Speech) | static_cast<std::uint32_t>(HeartRate);

constexpr auto kSupportedDetectionMask = 0U
#if UFEEL_ENABLE_EMOTIONS
                                         | static_cast<std::uint32_t>(Emotions)
#endif
#if UFEEL_ENABLE_EYE_TRACKING
                                         | static_cast<std::uint32_t>(EyeTracking)
#endif
#if UFEEL_ENABLE_SPEECH_TO_TEXT
                                         | static_cast<std::uint32_t>(Speech)
#endif
#if UFEEL_ENABLE_HEART_RATE
                                         | static_cast<std::uint32_t>(HeartRate)
#endif
    ;

constexpr auto kVideoDetectionMask = 0U
#if UFEEL_ENABLE_EMOTIONS
                                     | static_cast<std::uint32_t>(Emotions)
#endif
#if UFEEL_ENABLE_EYE_TRACKING
                                     | static_cast<std::uint32_t>(EyeTracking)
#endif
    ;

constexpr auto needsVideoFrame(
    std::uint32_t detectionType
) noexcept -> bool
{
    return (detectionType & kVideoDetectionMask) != 0U;
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

auto requireFrameReader(
    std::function<bool(cv::Mat&)> frameReader
) -> std::function<bool(cv::Mat&)>
{
    if (!frameReader)
    {
        throw std::invalid_argument("A frame reader is required.");
    }

    return frameReader;
}

} // namespace

DataProcessor::DataProcessor(
    const std::string& modelDirectory, const std::string& speechModelPath
)
    : DataProcessor(modelDirectory, speechModelPath, DataProcessorDevices{})
{
}

DataProcessor::DataProcessor(
    const std::string& modelDirectory, const std::string& speechModelPath, int cameraIndex
)
    : DataProcessor(
          modelDirectory, speechModelPath, DataProcessorDevices{.cameraIndex = cameraIndex}
      )
{
}

DataProcessor::DataProcessor(
    const std::string& modelDirectory,
    const std::string& speechModelPath,
    std::function<bool(cv::Mat&)> frameReader
)
    : DataProcessor(
          modelDirectory,
          speechModelPath,
          DataProcessorDevices{.frameReader = requireFrameReader(std::move(frameReader))}
      )
{
}

DataProcessor::DataProcessor(
    const std::string& modelDirectory,
    const std::string& speechModelPath,
    DataProcessorDevices devices
)
    : frameReader_(std::move(devices.frameReader))
{
#if UFEEL_ENABLE_EMOTIONS
    emotionDetector_ = std::make_unique<EmotionDetector>(modelDirectory);
#endif
#if UFEEL_ENABLE_EYE_TRACKING
    eyeTrackingDetector_ = std::make_unique<EyeTrackingDetector>(modelDirectory);
#endif
#if UFEEL_ENABLE_SPEECH_TO_TEXT
    speechToTextDetector_ = devices.audioSource
                                ? std::make_unique<SpeechToTextDetector>(
                                      speechModelPath, std::move(devices.audioSource)
                                  )
                                : std::make_unique<SpeechToTextDetector>(speechModelPath);
#endif
#if UFEEL_ENABLE_HEART_RATE
    heartRateSensorDetector_ =
        devices.heartRateSources.empty()
            ? std::make_unique<HeartRateSensorDetector>()
            : std::make_unique<HeartRateSensorDetector>(std::move(devices.heartRateSources));
#endif

    if (frameReader_)
    {
        return;
    }

    if (!cap_.open(devices.cameraIndex))
    {
        std::cerr << "[ERROR] Cannot open camera capture_id=" << devices.cameraIndex << "\n";
    }
    frameReader_ = [this](cv::Mat& frame) -> bool { return cap_.read(frame); };
}

auto DataProcessor::updateFrame() -> DataProcessorStatus
{
    cv::Mat capturedFrame;
    if (!frameReader_(capturedFrame) || capturedFrame.empty())
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
    if ((detectionType & ~kSupportedDetectionMask) != 0)
    {
        return DataProcessorStatus::UnsupportedFeature;
    }

    if (needsVideoFrame(detectionType))
    {
        const auto frameStatus = setFrame(image);
        if (frameStatus != DataProcessorStatus::Ok)
        {
            if (hasDetection(detectionType, Emotions))
            {
#if UFEEL_ENABLE_EMOTIONS
                emotions_ = {.status = EmotionStatus::FrameUnavailable};
#endif
            }
            if (hasDetection(detectionType, EyeTracking))
            {
#if UFEEL_ENABLE_EYE_TRACKING
                eyeTracking_ = {.status = EyeTrackingStatus::FrameUnavailable};
#endif
            }
            return frameStatus;
        }
    }

    if (hasDetection(detectionType, Emotions))
    {
#if UFEEL_ENABLE_EMOTIONS
        emotions_ = makeEmotionResult(emotionDetector_->process(frame_));
#else
        return DataProcessorStatus::UnsupportedFeature;
#endif
    }

    if (hasDetection(detectionType, EyeTracking))
    {
#if UFEEL_ENABLE_EYE_TRACKING
        eyeTracking_ = eyeTrackingDetector_->process(frame_);
#else
        return DataProcessorStatus::UnsupportedFeature;
#endif
    }

    if (hasDetection(detectionType, Speech))
    {
#if UFEEL_ENABLE_SPEECH_TO_TEXT
        const auto text = speechToTextDetector_->process();
        SpeechToTextStatus status = SpeechToTextStatus::Valid;
        if (!speechToTextEnabled_)
        {
            status = SpeechToTextStatus::Disabled;
        }
        else if (text.empty())
        {
            status = SpeechToTextStatus::NoSpeech;
        }

        speechToText_ = {
            .status = status,
            .text = speechToTextEnabled_ ? text : std::string{},
        };
#else
        return DataProcessorStatus::UnsupportedFeature;
#endif
    }

    if (hasDetection(detectionType, HeartRate))
    {
#if UFEEL_ENABLE_HEART_RATE
        heartRate_ = heartRateSensorDetector_->process();
#else
        return DataProcessorStatus::UnsupportedFeature;
#endif
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
    if ((detectionType & ~kSupportedDetectionMask) != 0)
    {
        return DataProcessorStatus::UnsupportedFeature;
    }

    if (needsVideoFrame(detectionType))
    {
        const auto frameStatus = updateFrame();
        if (frameStatus != DataProcessorStatus::Ok)
        {
            if (hasDetection(detectionType, Emotions))
            {
#if UFEEL_ENABLE_EMOTIONS
                emotions_ = {.status = EmotionStatus::FrameUnavailable};
#endif
            }
            if (hasDetection(detectionType, EyeTracking))
            {
#if UFEEL_ENABLE_EYE_TRACKING
                eyeTracking_ = {.status = EyeTrackingStatus::FrameUnavailable};
#endif
            }
            return frameStatus;
        }
    }

    return updateFromFrame(detectionType, frame_);
}

auto DataProcessor::updateEmotions() -> DataProcessorStatus
{
#if !UFEEL_ENABLE_EMOTIONS
    return DataProcessorStatus::UnsupportedFeature;
#else
    const auto frameStatus = updateFrame();
    if (frameStatus != DataProcessorStatus::Ok)
    {
        emotions_ = {.status = EmotionStatus::FrameUnavailable};
        return frameStatus;
    }

    emotions_ = makeEmotionResult(emotionDetector_->process(frame_));
    return DataProcessorStatus::Ok;
#endif
}

auto DataProcessor::updateEmotionsFromFrame(
    const cv::Mat& frame
) -> DataProcessorStatus
{
#if !UFEEL_ENABLE_EMOTIONS
    static_cast<void>(frame);
    return DataProcessorStatus::UnsupportedFeature;
#else
    const auto frameStatus = setFrame(frame);
    if (frameStatus != DataProcessorStatus::Ok)
    {
        emotions_ = {.status = EmotionStatus::FrameUnavailable};
        return frameStatus;
    }

    emotions_ = makeEmotionResult(emotionDetector_->process(frame_));
    return DataProcessorStatus::Ok;
#endif
}

auto DataProcessor::getEmotions() const -> const EmotionResult&
{
    return emotions_;
}

auto DataProcessor::getEmotionsFromFrame(
    const cv::Mat& frame
) const -> EmotionResult
{
#if !UFEEL_ENABLE_EMOTIONS
    static_cast<void>(frame);
    return {.status = EmotionStatus::FrameUnavailable};
#else
    if (frame.empty())
    {
        return {.status = EmotionStatus::FrameUnavailable};
    }

    return makeEmotionResult(emotionDetector_->process(frame));
#endif
}

auto DataProcessor::updateEyeTracking() -> DataProcessorStatus
{
#if !UFEEL_ENABLE_EYE_TRACKING
    return DataProcessorStatus::UnsupportedFeature;
#else
    const auto frameStatus = updateFrame();
    if (frameStatus != DataProcessorStatus::Ok)
    {
        eyeTracking_ = {.status = EyeTrackingStatus::FrameUnavailable};
        return frameStatus;
    }

    eyeTracking_ = eyeTrackingDetector_->process(frame_);
    return DataProcessorStatus::Ok;
#endif
}

auto DataProcessor::updateEyeTrackingFromFrame(
    const cv::Mat& frame
) -> DataProcessorStatus
{
#if !UFEEL_ENABLE_EYE_TRACKING
    static_cast<void>(frame);
    return DataProcessorStatus::UnsupportedFeature;
#else
    const auto frameStatus = setFrame(frame);
    if (frameStatus != DataProcessorStatus::Ok)
    {
        eyeTracking_ = {.status = EyeTrackingStatus::FrameUnavailable};
        return frameStatus;
    }

    eyeTracking_ = eyeTrackingDetector_->process(frame_);
    return DataProcessorStatus::Ok;
#endif
}

auto DataProcessor::getEyeTracking() const -> EyeTrackingResult
{
    return eyeTracking_;
}

auto DataProcessor::getEyeTrackingFromFrame(
    const cv::Mat& frame
) const -> EyeTrackingResult
{
#if !UFEEL_ENABLE_EYE_TRACKING
    static_cast<void>(frame);
    return {.status = EyeTrackingStatus::Disabled};
#else
    if (frame.empty())
    {
        return {.status = EyeTrackingStatus::FrameUnavailable};
    }

    return eyeTrackingDetector_->process(frame);
#endif
}

auto DataProcessor::updateSpeechToText() -> DataProcessorStatus
{
#if !UFEEL_ENABLE_SPEECH_TO_TEXT
    return DataProcessorStatus::UnsupportedFeature;
#else
    const auto text = speechToTextDetector_->process();
    SpeechToTextStatus status = SpeechToTextStatus::Valid;
    if (!speechToTextEnabled_)
    {
        status = SpeechToTextStatus::Disabled;
    }
    else if (text.empty())
    {
        status = SpeechToTextStatus::NoSpeech;
    }

    speechToText_ = {
        .status = status,
        .text = speechToTextEnabled_ ? text : std::string{},
    };
    return DataProcessorStatus::Ok;
#endif
}

auto DataProcessor::getSpeechToText() const -> const SpeechToTextResult&
{
    return speechToText_;
}

void DataProcessor::beginEyeTrackingCalibration()
{
#if UFEEL_ENABLE_EYE_TRACKING
    eyeTrackingDetector_->beginCalibration();
#endif
}

auto DataProcessor::setEyeTrackingCalibrationTarget(
    float x, float y
) -> bool
{
#if !UFEEL_ENABLE_EYE_TRACKING
    static_cast<void>(x);
    static_cast<void>(y);
    return false;
#else
    return eyeTrackingDetector_->setCalibrationTarget(x, y);
#endif
}

auto DataProcessor::finishEyeTrackingCalibration() -> bool
{
#if !UFEEL_ENABLE_EYE_TRACKING
    return false;
#else
    return eyeTrackingDetector_->finishCalibration();
#endif
}

void DataProcessor::cancelEyeTrackingCalibration()
{
#if UFEEL_ENABLE_EYE_TRACKING
    eyeTrackingDetector_->cancelCalibration();
#endif
}

auto DataProcessor::getEyeTrackingCalibrationProgress() const -> CalibrationProgress
{
#if !UFEEL_ENABLE_EYE_TRACKING
    return {};
#else
    return eyeTrackingDetector_->calibrationProgress();
#endif
}

auto DataProcessor::getEyeTrackingCalibrationDiagnostics() const -> CalibrationDiagnostics
{
#if !UFEEL_ENABLE_EYE_TRACKING
    return {};
#else
    return eyeTrackingDetector_->calibrationDiagnostics();
#endif
}

auto DataProcessor::saveEyeTrackingCalibrationProfile(
    const std::string& path
) const -> bool
{
#if !UFEEL_ENABLE_EYE_TRACKING
    static_cast<void>(path);
    return false;
#else
    return eyeTrackingDetector_->saveCalibrationProfile(path);
#endif
}

auto DataProcessor::loadEyeTrackingCalibrationProfile(
    const std::string& path
) -> bool
{
#if !UFEEL_ENABLE_EYE_TRACKING
    static_cast<void>(path);
    return false;
#else
    return eyeTrackingDetector_->loadCalibrationProfile(path);
#endif
}

auto DataProcessor::updateHeartRate() -> DataProcessorStatus
{
#if !UFEEL_ENABLE_HEART_RATE
    return DataProcessorStatus::UnsupportedFeature;
#else
    heartRate_ = heartRateSensorDetector_->process();
    return DataProcessorStatus::Ok;
#endif
}

auto DataProcessor::getHeartRate() const -> HeartRateResult
{
    return heartRate_;
}

void DataProcessor::toggleSpeechToText(
    bool state
)
{
#if !UFEEL_ENABLE_SPEECH_TO_TEXT
    static_cast<void>(state);
    speechToTextEnabled_ = false;
    speechToText_ = {.status = SpeechToTextStatus::Disabled};
#else
    speechToTextEnabled_ = state;
    if (!state)
    {
        speechToText_ = {.status = SpeechToTextStatus::Disabled};
    }

    speechToTextDetector_->toggle(state);
#endif
}

DataProcessor::~DataProcessor()
{
    if (cap_.isOpened())
    {
        cap_.release();
    }
}
