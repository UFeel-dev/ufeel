#include "DataProcessor.hpp"

#include "Emotions/EmotionDetector.hpp"
#include "EyeTracking/EyeTrackingCalibrationModel.hpp"
#include "EyeTracking/EyeTrackingDetector.hpp"
#include "HeartRate/HeartRateDetector.hpp"
#include "SpeechToText/SpeechToTextDetector.hpp"

#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <opencv2/core/mat.hpp>
#include <opencv2/videoio.hpp>
#include <string>

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

    // heartRateDetector_ = std::make_unique<HeartRateSensorDetector>();
}

auto DataProcessor::updateFrame() -> bool
{
    cap_ >> frame_;

    return !frame_.empty();
}

auto DataProcessor::setFrame(
    const cv::Mat& frame
) -> bool
{
    frame_ = frame;
    return true;
}

auto DataProcessor::getFrame() const -> cv::Mat
{
    return frame_;
}

auto DataProcessor::updateFromFrame(
    // TODO: check why this function always returns true, maybe it should return void instead of a
    // bool
    uint32_t detectionType,
    const cv::Mat& image
) -> bool
{
    frame_ = image;
    if ((detectionType & Emotions) != 0)
    {
        emotions_ = emotionDetector_->process(frame_);
    }
    if ((detectionType & EyeTracking) != 0)
    {
        eyeTracking_ = eyeTrackingDetector_->process(frame_);
    }
    if ((detectionType & Speech) != 0)
    {
        speechToText_ = speechToTextDetector_->process();
    }
    if ((detectionType & HeartRate) != 0)
    {
        heartRate_ = HeartRateSensorDetector::process();
    }
    return true;
}

auto DataProcessor::update(
    uint32_t detectionType
) -> bool
{
    if (((detectionType & Emotions) != 0 || (detectionType & EyeTracking) != 0) && (!updateFrame()))
    {
        return false;
    }

    return updateFromFrame(
        detectionType, frame_
    ); // ? why do we call this method even when only updating features that do not use the frame?
}

auto DataProcessor::updateEmotions() -> bool
{
    updateFrame();
    emotions_ = emotionDetector_->process(frame_);
    return true;
}

auto DataProcessor::updateEmotionsFromFrame(
    const cv::Mat& frame
) -> bool
{
    frame_ = frame;
    emotions_ = emotionDetector_->process(frame_);
    return true;
}

auto DataProcessor::getEmotions() const -> const std::map<std::string, float>&
{
    return emotions_;
}

auto DataProcessor::getEmotionsFromFrame(
    // TODO: fix const warning
    const cv::Mat& frame
) const -> const std::map<std::string, float>
{
    return emotionDetector_->process(frame);
}

auto DataProcessor::updateEyeTracking() -> bool
{
    updateFrame();
    eyeTracking_ = eyeTrackingDetector_->process(frame_);
    return true;
}

auto DataProcessor::updateEyeTrackingFromFrame(
    const cv::Mat& frame
) -> bool
{
    frame_ = frame;
    eyeTracking_ = eyeTrackingDetector_->process(frame_);
    return true;
}

auto DataProcessor::getEyeTracking() const -> EyeTrackingResult
{
    return eyeTracking_;
}

auto DataProcessor::getEyeTrackingFromFrame(
    const cv::Mat& frame
) const -> EyeTrackingResult
{
    return eyeTrackingDetector_->process(frame);
}

auto DataProcessor::updateSpeechToText() -> bool
{
    speechToText_ = speechToTextDetector_->process();
    return true;
}

auto DataProcessor::getSpeechToText() const -> const std::string&
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

auto DataProcessor::updateHeartRate() -> bool
{
    heartRate_ = HeartRateSensorDetector::process();
    return true;
}

auto DataProcessor::getHeartRate() const -> HeartRateResult
{
    return heartRate_;
}

void DataProcessor::toggleSpeechToText(
    bool state
)
{
    speechToTextDetector_->toggle(state);
}

DataProcessor::~DataProcessor()
{
    if (cap_.isOpened())
    {
        cap_.release();
    }
}
