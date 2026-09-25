/*
** EPITECH PROJECT, 2026
** test_lib
** File description:
** DataProcessor
*/

#ifndef DATAPROCESSOR_HPP_
    #define DATAPROCESSOR_HPP_

#include "Emotions/EmotionDetector.hpp"
#include "EyeTracking/EyeTrackingDetector.hpp"
#include "SpeechToText/SpeechToTextDetector.hpp"
// #include "HeartRateSensor/HeartRateSensorDetector.hpp"
#include <opencv2/opencv.hpp>


class DataProcessor
{
    public:
        DataProcessor();
        ~DataProcessor();

        DataProcessor(const DataProcessor&) = delete;
        DataProcessor& operator=(const DataProcessor&) = delete;

        DataProcessor(DataProcessor&&) = delete;
        DataProcessor& operator=(DataProcessor&&) = delete;

        std::map<std::string, float> processEmotion();

        void calibrateEyeTracking();
        std::map<std::string, bool> processEyeTracking();

        std::string processSpeechToText();
        void toggleSpeechToText(bool state);

        // int processHeartRateSensor();

        cv::Mat getFrame();

    protected:
        cv::VideoCapture cap_;
        cv::Mat frame_;

        std::unique_ptr<EmotionDetector> emotionDetector_;
        std::unique_ptr<EyeTrackingDetector> eyeTrackingDetector_;
        std::unique_ptr<SpeechToTextDetector> speechToTextDetector_;
        // std::unique_ptr<HeartRateSensorDetector> heartRateSensorDetector_;

    private:
};

#endif /* !DATAPROCESSOR_HPP_ */
