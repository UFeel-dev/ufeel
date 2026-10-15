#ifndef DATAPROCESSOR_HPP
#define DATAPROCESSOR_HPP

#include "HeartRate/HeartRateDetector.hpp"
#include "EyeTracking/EyeTrackingDetector.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/videoio.hpp"

#include <map>
#include <memory>
#include <string>

class EmotionDetector;
class SpeechToTextDetector;
class HeartRateSensorDetector;

class DataProcessor
{
    public:

        DataProcessor(const std::string& modelDirectory, const std::string& speechModelPath);

        ~DataProcessor();

        DataProcessor(const DataProcessor&) = delete;
        auto operator=(const DataProcessor&) -> DataProcessor& = delete;

        DataProcessor(DataProcessor&&) = delete;
        auto operator=(DataProcessor&&) -> DataProcessor& = delete;

        auto update() -> bool;

        [[nodiscard]] auto getFrame() const -> cv::Mat;

        [[nodiscard]] auto getEmotions() const -> const std::map<std::string, float>&;

        [[nodiscard]] auto getHeartRate() const -> HeartRateResult;

        void calibrateEyeTracking();
        [[nodiscard]] auto getGaze() const -> GazeResult;

        [[nodiscard]] auto getSpeech() const -> const std::string&;

        void beginEyeTrackingCalibration();
        [[nodiscard]] auto setEyeTrackingCalibrationTarget(float x, float y) -> bool;
        [[nodiscard]] auto finishEyeTrackingCalibration() -> bool;
        void cancelEyeTrackingCalibration();
        [[nodiscard]] auto getEyeTrackingCalibrationProgress() const -> CalibrationProgress;
        [[nodiscard]] auto saveEyeTrackingCalibrationProfile(const std::string& path) const -> bool;
        [[nodiscard]] auto loadEyeTrackingCalibrationProfile(const std::string& path) -> bool;

        void toggleSpeechToText(bool state);

    protected:

        cv::VideoCapture cap_;
        cv::Mat frame_;

        std::map<std::string, float> emotions_;
        GazeResult gaze_;
        std::string speech_;
        HeartRateResult heartrate_{};

        std::unique_ptr<EmotionDetector> emotionDetector_;
        std::unique_ptr<EyeTrackingDetector> eyeTrackingDetector_;
        std::unique_ptr<SpeechToTextDetector> speechToTextDetector_;
        // std::unique_ptr<HeartRateSensorDetector> heartRateSensorDetector_;

    private:
};

#endif // DATAPROCESSOR_HPP
