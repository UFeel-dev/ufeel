/*
** EPITECH PROJECT, 2026
** test_lib
** File description:
** DataProcessor
*/

#include "DataProcessor.hpp"
#include "Emotions/EmotionDetector.hpp"
#include "EyeTracking/EyeTrackingDetector.hpp"
#include "SpeechToText/SpeechToTextDetector.hpp"
#include "opencv2/core/mat.hpp"
#include <iostream>
#include <map>
#include <memory>
#include <string>

DataProcessor::DataProcessor() :
    cap_(0)
{
    if (!cap_.isOpened())
    {
        std::cerr << "[ERROR] Cannot open camera capture_id=0\n";
    }

    emotionDetector_ = std::make_unique<EmotionDetector>();
    eyeTrackingDetector_ = std::make_unique<EyeTrackingDetector>();
    speechToTextDetector_ = std::make_unique<SpeechToTextDetector>();
    // heartRateSensorDetector_ = std::make_unique<HeartRateSensorDetector>();
}

cv::Mat DataProcessor::getFrame()
{
    cap_ >> frame_;
    return frame_;
}

std::map<std::string, float> DataProcessor::processEmotion()
{
    cap_ >> frame_;
    if (frame_.empty()) {
        return {}; // TODO: add default value for all of this
    }

    return emotionDetector_->process(frame_);
}

void DataProcessor::calibrateEyeTracking()
{

}

std::map<std::string, bool> DataProcessor::processEyeTracking()
{
    cap_ >> frame_;
    if (frame_.empty()) {
        return {};
    }

    return eyeTrackingDetector_->process(frame_);
}

void DataProcessor::toggleSpeechToText(bool state)
{
    speechToTextDetector_->toggle(state);
}

std::string DataProcessor::processSpeechToText()
{
    return speechToTextDetector_->process();
}

// int DataProcessor::processHeartRateSensor()
// {
    // return heartRateSensorDetector_->process();
//     return 0;
// }

DataProcessor::~DataProcessor()
{
    if (cap_.isOpened()) {
        cap_.release();
    }

    emotionDetector_->close();
    eyeTrackingDetector_->close();
    // speechToTextDetector_->close();
    // heartRateSensorDetector_->close();
}
