#ifndef INTERNAL_UFEELPROCESSOR_HPP
#define INTERNAL_UFEELPROCESSOR_HPP

#include "DataProcessor.hpp"
#include "EyeTracking/EyeTrackingCalibrationModel.hpp"
#include "EyeTracking/EyeTrackingDetector.hpp"
#include "HeartRate/HeartRateSource.hpp"
#include "WrapperAllocator.hpp"
#include "opencv2/core/mat.hpp"
#include "ufeel.h"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace ufeel::internal
{

class ProcessorBackend
{
public:

    ProcessorBackend() = default;
    virtual ~ProcessorBackend() = default;
    ProcessorBackend(const ProcessorBackend&) = delete;
    auto operator=(const ProcessorBackend&) -> ProcessorBackend& = delete;
    ProcessorBackend(ProcessorBackend&&) = delete;
    auto operator=(ProcessorBackend&&) -> ProcessorBackend& = delete;

    virtual auto update(std::uint32_t detectionType) -> DataProcessorStatus = 0;
    virtual auto updateFromFrame(std::uint32_t detectionType, const cv::Mat& frame)
        -> DataProcessorStatus = 0;
    virtual auto updateFrame() -> DataProcessorStatus = 0;
    virtual auto setFrame(const cv::Mat& frame) -> DataProcessorStatus = 0;
    [[nodiscard]] virtual auto getFrame() const -> cv::Mat = 0;

    virtual auto updateEmotions() -> DataProcessorStatus = 0;
    virtual auto updateEmotionsFromFrame(const cv::Mat& frame) -> DataProcessorStatus = 0;
    [[nodiscard]] virtual auto getEmotions() const -> const EmotionResult& = 0;
    [[nodiscard]] virtual auto getEmotionsFromFrame(const cv::Mat& frame) const
        -> EmotionResult = 0;

    virtual auto updateEyeTracking() -> DataProcessorStatus = 0;
    virtual auto updateEyeTrackingFromFrame(const cv::Mat& frame) -> DataProcessorStatus = 0;
    [[nodiscard]] virtual auto getEyeTracking() const -> EyeTrackingResult = 0;
    [[nodiscard]] virtual auto getEyeTrackingFromFrame(const cv::Mat& frame) const
        -> EyeTrackingResult = 0;

    virtual auto updateSpeechToText() -> DataProcessorStatus = 0;
    [[nodiscard]] virtual auto getSpeechToText() const -> const SpeechToTextResult& = 0;
    virtual void toggleSpeechToText(bool enabled) = 0;

    virtual auto updateHeartRate() -> DataProcessorStatus = 0;
    [[nodiscard]] virtual auto getHeartRate() const -> HeartRateResult = 0;

    virtual void beginEyeTrackingCalibration() = 0;
    [[nodiscard]] virtual auto setEyeTrackingCalibrationTarget(float x, float y) -> bool = 0;
    [[nodiscard]] virtual auto finishEyeTrackingCalibration() -> bool = 0;
    virtual void cancelEyeTrackingCalibration() = 0;
    [[nodiscard]] virtual auto getEyeTrackingCalibrationProgress() const -> CalibrationProgress = 0;
    [[nodiscard]] virtual auto getEyeTrackingCalibrationDiagnostics() const
        -> CalibrationDiagnostics = 0;
    [[nodiscard]] virtual auto saveEyeTrackingCalibrationProfile(const std::string& path) const
        -> bool = 0;
    [[nodiscard]] virtual auto loadEyeTrackingCalibrationProfile(const std::string& path)
        -> bool = 0;
};

constexpr auto toUFeelStatus(
    DataProcessorStatus status
) noexcept -> UFeelStatus
{
    switch (status)
    {
    case DataProcessorStatus::Ok:
        return UFEEL_STATUS_OK;
    case DataProcessorStatus::InvalidFrame:
    case DataProcessorStatus::FrameUnavailable:
        return UFEEL_STATUS_INVALID_FRAME;
    case DataProcessorStatus::InvalidDetectionType:
        return UFEEL_STATUS_INVALID_ARGUMENT;
    case DataProcessorStatus::UnsupportedFeature:
        return UFEEL_STATUS_UNSUPPORTED_FEATURE;
    }

    return UFEEL_STATUS_INTERNAL_ERROR;
}

} // namespace ufeel::internal

struct UFeelProcessor
{
    std::unique_ptr<ufeel::internal::ProcessorBackend> processor;
    ufeel::internal::WrapperAllocator* allocator = &ufeel::internal::defaultWrapperAllocator();

    UFeelProcessor(const std::string& modelDirectory, const std::string& speechModelPath);
    explicit UFeelProcessor(
        std::unique_ptr<ufeel::internal::ProcessorBackend> backend,
        ufeel::internal::WrapperAllocator& resultAllocator =
            ufeel::internal::defaultWrapperAllocator()
    )
        : processor(std::move(backend))
        , allocator(&resultAllocator)
    {
    }
};

#endif // INTERNAL_UFEELPROCESSOR_HPP
