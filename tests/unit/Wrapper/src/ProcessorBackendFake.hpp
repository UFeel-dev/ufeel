#ifndef TESTS_UNIT_WRAPPER_PROCESSORBACKENDFAKE_HPP
#define TESTS_UNIT_WRAPPER_PROCESSORBACKENDFAKE_HPP

#include "DataProcessor.hpp"
#include "EyeTracking/EyeTrackingCalibrationModel.hpp"
#include "EyeTracking/EyeTrackingDetector.hpp"
#include "HeartRate/HeartRateSource.hpp"
#include "HeartRate/UFeelHeartRateStatus.hpp"
#include "internal/UFeelProcessor.hpp"
#include "opencv2/core/mat.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>

class ProcessorBackendFake final : public ufeel::internal::ProcessorBackend
{
public:

    DataProcessorStatus updateStatus = DataProcessorStatus::Ok;
    DataProcessorStatus updateFrameStatus = DataProcessorStatus::Ok;
    DataProcessorStatus setFrameStatus = DataProcessorStatus::Ok;
    DataProcessorStatus emotionStatus = DataProcessorStatus::Ok;
    DataProcessorStatus eyeTrackingStatus = DataProcessorStatus::Ok;
    DataProcessorStatus speechStatus = DataProcessorStatus::Ok;
    DataProcessorStatus heartRateStatus = DataProcessorStatus::Ok;

    cv::Mat frame;
    EmotionResult emotions{.status = EmotionStatus::Valid, .emotions = {{"joy", 0.75F}}};
    EmotionResult frameEmotions = emotions;
    EyeTrackingResult eyeTracking{
        .x = 0.2F,
        .y = 0.7F,
        .recordedX = 0.3F,
        .recordedY = 0.6F,
        .confidence = 0.9F,
        .directions = EyeTrackingLeft | EyeTrackingUp,
        .status = EyeTrackingStatus::Valid,
    };
    EyeTrackingResult frameEyeTracking = eyeTracking;
    SpeechToTextResult speech{.status = SpeechToTextStatus::Valid, .text = "recognized"};
    HeartRateResult heartRate{.bpm = 73, .status = HeartRateStatus::Valid};
    CalibrationProgress calibrationProgress{
        .targetsWithEnoughSamples = 2,
        .targetCount = 3,
        .samplesForCurrentTarget = 4,
        .requiredSamplesPerTarget = 5,
    };
    CalibrationDiagnostics calibrationDiagnostics{
        .failureReason = CalibrationFailureReason::InsufficientSamples,
        .inputTargetCount = 3,
        .targetsWithEnoughRawSamples = 2,
        .targetsWithEnoughRetainedSamples = 1,
        .validTargetCount = 1,
        .minimumRetainedSamples = 4,
        .maximumRetainedSamples = 8,
        .horizontalSpan = 0.6,
        .verticalSpan = 0.7,
        .meanCrossValidationError = 0.1,
        .maximumCrossValidationError = 0.2,
        .crossValidationTargetsEvaluated = 2,
        .meanErrorExceeded = true,
        .maximumErrorExceeded = false,
        .regressionFailures = 1,
        .invalidTargetCount = 2,
        .targetsWithInsufficientRawSamples = 3,
        .targetsWithInsufficientRetainedSamples = 4,
    };

    bool calibrationTargetAccepted = true;
    bool calibrationFinished = true;
    bool profileSaved = true;
    bool profileLoaded = true;
    bool speechEnabled = false;
    bool throwOnUpdate = false;
    bool throwUnknownOnUpdate = false;
    bool throwOnFrame = false;
    bool throwOnEmotion = false;
    bool throwOnEyeTracking = false;
    bool throwOnSpeech = false;
    bool throwOnHeartRate = false;
    bool throwOnCalibration = false;

    auto update(
        std::uint32_t detectionType
    ) -> DataProcessorStatus override
    {
        static_cast<void>(detectionType);
        if (throwOnUpdate)
        {
            throw std::runtime_error("fake update failure");
        }
        if (throwUnknownOnUpdate)
        {
            throw 1;
        }
        return updateStatus;
    }

    auto updateFromFrame(
        std::uint32_t detectionType, const cv::Mat& inputFrame
    ) -> DataProcessorStatus override
    {
        static_cast<void>(detectionType);
        static_cast<void>(inputFrame);
        return updateStatus;
    }

    auto updateFrame() -> DataProcessorStatus override
    {
        return updateFrameStatus;
    }

    auto setFrame(
        const cv::Mat& value
    ) -> DataProcessorStatus override
    {
        if (setFrameStatus == DataProcessorStatus::Ok)
        {
            frame = value.clone();
        }
        return setFrameStatus;
    }

    [[nodiscard]] auto getFrame() const -> cv::Mat override
    {
        throwIfEnabled(throwOnFrame);
        return frame;
    }

    auto updateEmotions() -> DataProcessorStatus override
    {
        throwIfEnabled(throwOnEmotion);
        return emotionStatus;
    }

    auto updateEmotionsFromFrame(
        const cv::Mat& inputFrame
    ) -> DataProcessorStatus override
    {
        static_cast<void>(inputFrame);
        throwIfEnabled(throwOnEmotion);
        return emotionStatus;
    }

    [[nodiscard]] auto getEmotions() const -> const EmotionResult& override
    {
        throwIfEnabled(throwOnEmotion);
        return emotions;
    }

    [[nodiscard]] auto getEmotionsFromFrame(
        const cv::Mat& inputFrame
    ) const -> EmotionResult override
    {
        static_cast<void>(inputFrame);
        throwIfEnabled(throwOnEmotion);
        return frameEmotions;
    }

    auto updateEyeTracking() -> DataProcessorStatus override
    {
        throwIfEnabled(throwOnEyeTracking);
        return eyeTrackingStatus;
    }

    auto updateEyeTrackingFromFrame(
        const cv::Mat& inputFrame
    ) -> DataProcessorStatus override
    {
        static_cast<void>(inputFrame);
        throwIfEnabled(throwOnEyeTracking);
        return eyeTrackingStatus;
    }

    [[nodiscard]] auto getEyeTracking() const -> EyeTrackingResult override
    {
        throwIfEnabled(throwOnEyeTracking);
        return eyeTracking;
    }

    [[nodiscard]] auto getEyeTrackingFromFrame(
        const cv::Mat& inputFrame
    ) const -> EyeTrackingResult override
    {
        static_cast<void>(inputFrame);
        throwIfEnabled(throwOnEyeTracking);
        return frameEyeTracking;
    }

    auto updateSpeechToText() -> DataProcessorStatus override
    {
        throwIfEnabled(throwOnSpeech);
        return speechStatus;
    }

    [[nodiscard]] auto getSpeechToText() const -> const SpeechToTextResult& override
    {
        throwIfEnabled(throwOnSpeech);
        return speech;
    }

    void toggleSpeechToText(
        bool enabled
    ) override
    {
        speechEnabled = enabled;
    }

    auto updateHeartRate() -> DataProcessorStatus override
    {
        throwIfEnabled(throwOnHeartRate);
        return heartRateStatus;
    }

    [[nodiscard]] auto getHeartRate() const -> HeartRateResult override
    {
        throwIfEnabled(throwOnHeartRate);
        return heartRate;
    }

    void beginEyeTrackingCalibration() override
    {
        throwIfEnabled(throwOnCalibration);
    }

    [[nodiscard]] auto setEyeTrackingCalibrationTarget(
        float targetX, float targetY
    ) -> bool override
    {
        static_cast<void>(targetX);
        static_cast<void>(targetY);
        throwIfEnabled(throwOnCalibration);
        return calibrationTargetAccepted;
    }

    [[nodiscard]] auto finishEyeTrackingCalibration() -> bool override
    {
        throwIfEnabled(throwOnCalibration);
        return calibrationFinished;
    }

    void cancelEyeTrackingCalibration() override
    {
        throwIfEnabled(throwOnCalibration);
    }

    [[nodiscard]] auto getEyeTrackingCalibrationProgress() const -> CalibrationProgress override
    {
        throwIfEnabled(throwOnCalibration);
        return calibrationProgress;
    }

    [[nodiscard]] auto getEyeTrackingCalibrationDiagnostics() const
        -> CalibrationDiagnostics override
    {
        throwIfEnabled(throwOnCalibration);
        return calibrationDiagnostics;
    }

    [[nodiscard]] auto saveEyeTrackingCalibrationProfile(
        const std::string& path
    ) const -> bool override
    {
        static_cast<void>(path);
        throwIfEnabled(throwOnCalibration);
        return profileSaved;
    }

    [[nodiscard]] auto loadEyeTrackingCalibrationProfile(
        const std::string& path
    ) -> bool override
    {
        static_cast<void>(path);
        throwIfEnabled(throwOnCalibration);
        return profileLoaded;
    }

private:

    static void throwIfEnabled(
        bool enabled
    )
    {
        if (enabled)
        {
            throw std::runtime_error("fake processor failure");
        }
    }
};

#endif // TESTS_UNIT_WRAPPER_PROCESSORBACKENDFAKE_HPP
