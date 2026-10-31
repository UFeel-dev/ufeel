#include "DataProcessor.hpp"

#include "Emotions/EmotionDetector.hpp"
#include "EyeTracking/EyeTrackingDetector.hpp"
#include "EyeTracking/GazeCalibrationModel.hpp"
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
    if ((detectionType & Gaze) != 0)
    {
        gaze_ = eyeTrackingDetector_->process(frame_);
    }
    if ((detectionType & Speech) != 0)
    {
        speech_ = speechToTextDetector_->process();
    }
    if ((detectionType & HeartRate) != 0)
    {
        heartrate_ = heartRateDetector_->process();
    }
    return true;
}

auto DataProcessor::update(uint32_t detectionType) -> bool
{
    if ((detectionType & Emotions) != 0 || (detectionType & Gaze) != 0)
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

// TODO: Do we set the frame_ value in the Class to frame or we keep the current value
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

auto DataProcessor::updateGaze() -> bool
{
    updateFrame();
    gaze_ = eyeTrackingDetector_->process(frame_);
    return true;
}

auto DataProcessor::updateGazeFromFrame(const cv::Mat& frame) -> bool
{
    frame_ = frame;
    gaze_ = eyeTrackingDetector_->process(frame_);
    return true;
}

auto DataProcessor::getGaze() const -> GazeResult
{
    return gaze_;
}

auto DataProcessor::updateSpeech() -> bool
{
    speech_ = speechToTextDetector_->process();
    return true;
}

auto DataProcessor::getSpeech() const -> const std::string&
{
    return speech_;
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
