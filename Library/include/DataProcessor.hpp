#ifndef DATAPROCESSOR_HPP
#define DATAPROCESSOR_HPP

#include "EyeTracking/EyeTrackingDetector.hpp"
#include "EyeTracking/GazeCalibrationModel.hpp"
#include "HeartRate/HeartRateDetector.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/videoio.hpp"

#include <map>
#include <memory>
#include <string>

class EmotionDetector;
class SpeechToTextDetector;
class HeartRateSensorDetector;

enum DetectionType : std::uint8_t
{
    Emotions = 1U << 0U,
    Gaze = 1U << 1U,
    Speech = 1U << 2U,
    HeartRate = 1U << 3U,
};

class DataProcessor
{
public:

    DataProcessor(const std::string& modelDirectory, const std::string& speechModelPath);

    ~DataProcessor();

    DataProcessor(const DataProcessor&) = delete;
    auto operator=(const DataProcessor&) -> DataProcessor& = delete;

    DataProcessor(DataProcessor&&) = delete;
    auto operator=(DataProcessor&&) -> DataProcessor& = delete;

    auto updateFrame() -> bool;
    auto setFrame(const cv::Mat& frame) -> bool;
    [[nodiscard]] auto getFrame() const -> cv::Mat;

    auto updateFromFrame(uint32_t detectionType, const cv::Mat& image) -> bool;
    auto update(uint32_t detectionType) -> bool;

    auto updateEmotions() -> bool;
    auto updateEmotionsFromFrame(const cv::Mat& image) -> bool;
    [[nodiscard]] auto getEmotions() const -> const std::map<std::string, float>&;

    auto updateGaze() -> bool;
    auto updateGazeFromFrame(const cv::Mat& image) -> bool;
    [[nodiscard]] auto getGaze() const -> GazeResult;

    auto updateSpeech() -> bool;
    [[nodiscard]] auto getSpeech() const -> const std::string&;

    auto updateHeartRate() -> bool;
    [[nodiscard]] auto getHeartRate() const -> HeartRateResult;

    void beginEyeTrackingCalibration();
    [[nodiscard]] auto setEyeTrackingCalibrationTarget(float x, float y) -> bool;
    [[nodiscard]] auto finishEyeTrackingCalibration() -> bool;
    void cancelEyeTrackingCalibration();
    [[nodiscard]] auto getEyeTrackingCalibrationProgress() const -> CalibrationProgress;
    [[nodiscard]] auto getEyeTrackingCalibrationDiagnostics() const -> CalibrationDiagnostics;
    [[nodiscard]] auto saveEyeTrackingCalibrationProfile(const std::string& path) const -> bool;
    [[nodiscard]] auto loadEyeTrackingCalibrationProfile(const std::string& path) -> bool;

    void toggleSpeechToText(bool state);

private:

    cv::VideoCapture cap_;
    cv::Mat frame_;

    std::map<std::string, float> emotions_;
    GazeResult gaze_;
    std::string speech_;
    HeartRateResult heartrate_{};

    std::unique_ptr<EmotionDetector> emotionDetector_;
    std::unique_ptr<EyeTrackingDetector> eyeTrackingDetector_;
    std::unique_ptr<SpeechToTextDetector> speechToTextDetector_;
    std::unique_ptr<HeartRateSensorDetector> heartRateDetector_;
};

#endif // DATAPROCESSOR_HPP
