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

        // auto processHeartRateSensor() -> int;

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
