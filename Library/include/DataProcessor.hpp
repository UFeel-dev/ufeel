#ifndef DATAPROCESSOR_HPP
#define DATAPROCESSOR_HPP

#include "EyeTracking/EyeTrackingCalibrationModel.hpp"
#include "EyeTracking/EyeTrackingDetector.hpp"
#include "HeartRate/HeartRateDetector.hpp"
#include "HeartRate/HeartRateSource.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/videoio.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>

class EmotionDetector;
class SpeechToTextDetector;
class HeartRateSensorDetector;

enum DetectionType : std::uint8_t // NOLINT(cppcoreguidelines-use-enum-class): unscoped bit flags
                                  // are part of the existing public API.
{
    None = 0U,
    Emotions = 1U << 0U,
    EyeTracking = 1U << 1U,
    Speech = 1U << 2U,
    HeartRate = 1U << 3U,
};

enum class DataProcessorStatus : std::uint8_t
{
    Ok,
    InvalidFrame,
    FrameUnavailable,
    InvalidDetectionType,
};

enum class EmotionStatus : std::uint8_t
{
    FaceUnavailable,
    Valid,
    FrameUnavailable,
};

struct EmotionResult
{
    EmotionStatus status = EmotionStatus::FrameUnavailable;
    std::map<std::string, float> emotions;
};

enum class SpeechToTextStatus : std::uint8_t
{
    Disabled,
    NoSpeech,
    Valid,
};

struct SpeechToTextResult
{
    SpeechToTextStatus status = SpeechToTextStatus::Disabled;
    std::string text;
};

constexpr auto hasDetection(
    std::uint32_t detectionType, DetectionType flag
) noexcept -> bool
{
    return (detectionType & static_cast<std::uint32_t>(flag)) != 0U;
}

class DataProcessor
{
public:

    DataProcessor(const std::string& modelDirectory, const std::string& speechModelPath);
    DataProcessor(
        const std::string& modelDirectory, const std::string& speechModelPath, int cameraIndex
    );
    DataProcessor(
        const std::string& modelDirectory,
        const std::string& speechModelPath,
        std::function<bool(cv::Mat&)> frameReader
    );

    ~DataProcessor();

    DataProcessor(const DataProcessor&) = delete;
    auto operator=(const DataProcessor&) -> DataProcessor& = delete;

    DataProcessor(DataProcessor&&) = delete;
    auto operator=(DataProcessor&&) -> DataProcessor& = delete;

    auto updateFrame() -> DataProcessorStatus;
    auto setFrame(const cv::Mat& frame) -> DataProcessorStatus;
    [[nodiscard]] auto getFrame() const -> cv::Mat;

    auto updateFromFrame(uint32_t detectionType, const cv::Mat& image) -> DataProcessorStatus;
    auto update(uint32_t detectionType) -> DataProcessorStatus;

    auto updateEmotions() -> DataProcessorStatus;
    auto updateEmotionsFromFrame(const cv::Mat& frame) -> DataProcessorStatus;
    [[nodiscard]] auto getEmotions() const -> const EmotionResult&;
    [[nodiscard]] auto getEmotionsFromFrame(const cv::Mat& frame) const -> EmotionResult;

    auto updateEyeTracking() -> DataProcessorStatus;
    auto updateEyeTrackingFromFrame(const cv::Mat& frame) -> DataProcessorStatus;
    [[nodiscard]] auto getEyeTracking() const -> EyeTrackingResult;
    [[nodiscard]] auto getEyeTrackingFromFrame(const cv::Mat& frame) const -> EyeTrackingResult;

    auto updateSpeechToText() -> DataProcessorStatus;
    [[nodiscard]] auto getSpeechToText() const -> const SpeechToTextResult&;

    auto updateHeartRate() -> DataProcessorStatus;
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

    void initializeDetectors(const std::string& modelDirectory, const std::string& speechModelPath);

    cv::VideoCapture cap_;
    std::function<bool(cv::Mat&)> frameReader_;
    cv::Mat frame_;

    EmotionResult emotions_;
    EyeTrackingResult eyeTracking_;
    SpeechToTextResult speechToText_;
    HeartRateResult heartRate_{};
    bool speechToTextEnabled_ = false;

    std::unique_ptr<EmotionDetector> emotionDetector_;
    std::unique_ptr<EyeTrackingDetector> eyeTrackingDetector_;
    std::unique_ptr<SpeechToTextDetector> speechToTextDetector_;
    std::unique_ptr<HeartRateSensorDetector> heartRateSensorDetector_;
};

#endif // DATAPROCESSOR_HPP
