#include "DataProcessor.hpp"

#include "Emotions/EmotionDetector.hpp"
#include "EyeTracking/EyeTrackingDetector.hpp"
#include "SpeechToText/SpeechToTextDetector.hpp"

#include <iostream>
#include <map>
#include <memory>
#include <string>

DataProcessor::DataProcessor(
    const std::string& speechModelPath
)
    : cap_(0)
{
    if (!cap_.isOpened())
    {
        std::cerr << "[ERROR] Cannot open camera capture_id=0\n";
    }

    emotionDetector_ = std::make_unique<EmotionDetector>();

    eyeTrackingDetector_ = std::make_unique<EyeTrackingDetector>();

    speechToTextDetector_ = std::make_unique<SpeechToTextDetector>(speechModelPath);
}

auto DataProcessor::update() -> bool
{
    cap_ >> frame_;

    if (frame_.empty())
    {
        emotions_.clear();
        directions_.clear();
        return false;
    }

    emotions_ = emotionDetector_->process(frame_);

    directions_ = eyeTrackingDetector_->process(frame_);

    speech_ = speechToTextDetector_->process();

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

auto DataProcessor::getDirections() const -> const std::map<std::string, bool>&
{
    return directions_;
}

auto DataProcessor::getSpeech() const -> const std::string&
{
    return speech_;
}

void DataProcessor::calibrateEyeTracking() {}

void DataProcessor::toggleSpeechToText(
    bool state
)
{
    speechToTextDetector_->toggle(state);
}

// int DataProcessor::processHeartRateSensor()
// {
// return heartRateSensorDetector_->process();
//     return 0;
// }

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
