#ifndef DATAPROCESSOR_HPP
#define DATAPROCESSOR_HPP

#include "EyeTracking/EyeTrackingDetector.hpp"
#include "EyeTracking/GazeCalibrationModel.hpp"
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

    [[nodiscard]] auto getGaze() const -> GazeResult;

    [[nodiscard]] auto getSpeech() const -> const std::string&;

    [[nodiscard]] auto getHeartRate() const -> HeartRateResult;

    [[nodiscard]] auto getHeartRate() const -> int;

    std::unique_ptr<EmotionDetector> emotionDetector_;
    std::unique_ptr<EyeTrackingDetector> eyeTrackingDetector_;
    std::unique_ptr<SpeechToTextDetector> speechToTextDetector_;
    // std::unique_ptr<HeartRateSensorDetector> heartRateSensorDetector_;

    private:
};

#endif // DATAPROCESSOR_HPP
