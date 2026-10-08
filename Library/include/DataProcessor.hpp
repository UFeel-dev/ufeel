#ifndef DATAPROCESSOR_HPP
#define DATAPROCESSOR_HPP

#include "opencv2/core/mat.hpp"
#include "opencv2/videoio.hpp"

#include <map>
#include <memory>
#include <string>

class EmotionDetector;
class EyeTrackingDetector;
class SpeechToTextDetector;

class DataProcessor
{
    public:

        DataProcessor(
            const std::string& speechModelPath
        );

        ~DataProcessor();

        DataProcessor(const DataProcessor&) = delete;
        auto operator=(const DataProcessor&) -> DataProcessor& = delete;

        DataProcessor(DataProcessor&&) = delete;
        auto operator=(DataProcessor&&) -> DataProcessor& = delete;

        auto update() -> bool;

        auto getFrame() const -> cv::Mat;

        auto getEmotions() const
            -> const std::map<std::string, float>&;

        auto getDirections() const
            -> const std::map<std::string, bool>&;

        auto getSpeech() const -> const std::string&;

        void calibrateEyeTracking();

        void toggleSpeechToText(bool state);

        // auto processHeartRateSensor() -> int;

    protected:

        cv::VideoCapture cap_;
        cv::Mat frame_;

        std::map<std::string, float> emotions_;
        std::map<std::string, bool> directions_;
        std::string speech_;

        std::unique_ptr<EmotionDetector> emotionDetector_;
        std::unique_ptr<EyeTrackingDetector> eyeTrackingDetector_;
        std::unique_ptr<SpeechToTextDetector> speechToTextDetector_;
        // std::unique_ptr<HeartRateSensorDetector> heartRateSensorDetector_;

    private:
};

#endif // DATAPROCESSOR_HPP
