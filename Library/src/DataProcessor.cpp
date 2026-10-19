#include "DataProcessor.hpp"

#include "Emotions/EmotionDetector.hpp"
#include "EyeTracking/EyeTrackingDetector.hpp"
#include "EyeTracking/GazeCalibrationModel.hpp"
#include "HeartRate/HeartRateDetector.hpp"
#include "SpeechToText/SpeechToTextDetector.hpp"

#include <filesystem>
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

    auto const modelPath = std::filesystem::path(modelDirectory);
    emotionDetector_ = std::make_unique<EmotionDetector>(modelPath);

    eyeTrackingDetector_ = std::make_unique<EyeTrackingDetector>(modelDirectory);

    speechToTextDetector_ = std::make_unique<SpeechToTextDetector>(speechModelPath);

    // heartRateSensorDetector_ = std::make_unique<HeartRateSensorDetector>();
}

auto DataProcessor::update() -> bool
{
    cap_ >> frame_;

    if (frame_.empty())
    {
        emotions_.clear();
        gaze_ = {};
        gaze_.status = GazeStatus::FrameUnavailable;
        return false;
    }

    emotions_ = emotionDetector_->process(frame_);

    gaze_ = eyeTrackingDetector_->process(frame_);

    speech_ = speechToTextDetector_->process();

    heartrate_ = HeartRateSensorDetector::process();

    return true;
}

auto DataProcessor::getFrame() const -> cv::Mat
{
    return frame_;
}

auto DataProcessor::getEmotions() const -> const std::map<std::string, float>&
{
    return emotions_;
}

auto DataProcessor::getGaze() const -> GazeResult
{
    return gaze_;
}

auto DataProcessor::getSpeech() const -> const std::string&
{
    return speech_;
}

auto DataProcessor::getHeartRate() const -> HeartRateResult
{
    return heartrate_;
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

auto DataProcessor::getHeartRate() const -> int
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

    // emotionDetector_->close();
    // eyeTrackingDetector_->close();
    // speechToTextDetector_->close();
    // heartRateSensorDetector_->close();
}
