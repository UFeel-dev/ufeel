/*
** EPITECH PROJECT, 2026
** test_lib
** File description:
** DataProcessor
*/

#ifndef DATAPROCESSOR_HPP
    #define DATAPROCESSOR_HPP

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
        auto operator=(const DataProcessor&) -> DataProcessor& = delete;

        DataProcessor(DataProcessor&&) = delete;
        auto operator=(DataProcessor&&) -> DataProcessor& = delete;

        auto processEmotion() -> std::map<std::string, float>;

        void calibrateEyeTracking();
        auto processEyeTracking() -> std::map<std::string, bool>;

        auto processSpeechToText() -> std::string;
        void toggleSpeechToText(bool state);

        // int processHeartRateSensor();

        auto getFrame() -> cv::Mat;

    protected:
        cv::VideoCapture cap_;
        cv::Mat frame_;

        std::unique_ptr<EmotionDetector> emotionDetector_;
        std::unique_ptr<EyeTrackingDetector> eyeTrackingDetector_;
        std::unique_ptr<SpeechToTextDetector> speechToTextDetector_;
        // std::unique_ptr<HeartRateSensorDetector> heartRateSensorDetector_;

    private:
};

#endif // DATAPROCESSOR_HPP 
