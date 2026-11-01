#include "DataProcessor.hpp"

#include "Emotions/EmotionDetector.hpp"
#include "EyeTracking/EyeTrackingDetector.hpp"
#include "EyeTracking/EyeTrackingCalibrationModel.hpp"
#include "HeartRate/HeartRateDetector.hpp"
#include "SpeechToText/SpeechToTextDetector.hpp"

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

    heartRateDetector_ = std::make_unique<HeartRateSensorDetector>();
}

auto DataProcessor::updateFrame() -> bool
{
    cap_ >> frame_;

    return !frame_.empty();
}

auto DataProcessor::setFrame(const cv::Mat& frame) -> bool
{
    frame_ = frame;
    return true;
}

auto DataProcessor::getFrame() const -> cv::Mat
{
    return frame_;
}

auto DataProcessor::updateFromFrame(uint32_t detectionType, const cv::Mat& image) -> bool
{
    frame_ = image;
    if ((detectionType & Emotions) != 0)
    {
        emotions_ = emotionDetector_->process(frame_);
    }
    if ((detectionType & EyeTracking) != 0)
    {
        eyetracking_ = eyeTrackingDetector_->process(frame_);
    }
    if ((detectionType & Speech) != 0)
    {
        speechtotext_ = speechToTextDetector_->process();
    }
    if ((detectionType & HeartRate) != 0)
    {
        heartrate_ = heartRateDetector_->process();
    }
    return true;
}

auto DataProcessor::update(uint32_t detectionType) -> bool
{
    if ((detectionType & Emotions) != 0 || (detectionType & EyeTracking) != 0)
    {
        updateFrame();
    }
    return updateFromFrame(detectionType, frame_);
}

auto DataProcessor::updateEmotions() -> bool
{
    updateFrame();
    emotions_ = emotionDetector_->process(frame_);
    return true;
}

auto DataProcessor::updateEmotionsFromFrame(const cv::Mat& frame) -> bool
{
    frame_ = frame;
    emotions_ = emotionDetector_->process(frame_);
    return true;
}

auto DataProcessor::getEmotions() const -> const std::map<std::string, float>&
{
    return emotions_;
}

auto DataProcessor::getEmotionsFromFrame(const cv::Mat& frame) const -> const std::map<std::string, float>
{
    return emotionDetector_->process(frame);
}

auto DataProcessor::updateEyeTracking() -> bool
{
    updateFrame();
    eyetracking_ = eyeTrackingDetector_->process(frame_);
    return true;
}

auto DataProcessor::updateEyeTrackingFromFrame(const cv::Mat& frame) -> bool
{
    frame_ = frame;
    eyetracking_ = eyeTrackingDetector_->process(frame_);
    return true;
}

auto DataProcessor::getEyeTracking() const -> EyeTrackingResult
{
    return eyetracking_;
}

auto DataProcessor::getEyeTrackingFromFrame(const cv::Mat& frame) const -> EyeTrackingResult
{
    return eyeTrackingDetector_->process(frame);
}


auto DataProcessor::updateSpeechToText() -> bool
{
    speechtotext_ = speechToTextDetector_->process();
    return true;
}

auto DataProcessor::getSpeechToText() const -> const std::string&
{
    return speechtotext_;
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
    heartrate_ = heartRateDetector_->process();
    return true;
}

auto DataProcessor::getHeartRate() const -> HeartRateResult
{
    return heartrate_;
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
