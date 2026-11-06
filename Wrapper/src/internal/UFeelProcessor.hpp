#ifndef UFEELPROCESSOR_HPP
#define UFEELPROCESSOR_HPP

#include "DataProcessor.hpp"
#include "EyeTracking/EyeTrackingCalibrationModel.hpp"
#include "EyeTracking/EyeTrackingDetector.hpp"
#include "HeartRate/HeartRateSource.hpp"
#include "Memory.hpp"
#include "opencv2/core/mat.hpp"
#include "ufeel.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <new>
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

class WrapperAllocator
{
public:

    WrapperAllocator() = default;
    virtual ~WrapperAllocator() = default;
    WrapperAllocator(const WrapperAllocator&) = delete;
    auto operator=(const WrapperAllocator&) -> WrapperAllocator& = delete;
    WrapperAllocator(WrapperAllocator&&) = delete;
    auto operator=(WrapperAllocator&&) -> WrapperAllocator& = delete;

    [[nodiscard]] virtual auto createFrame() noexcept -> UFeelFrame* = 0;
    [[nodiscard]] virtual auto allocateFrameData(std::size_t size) noexcept -> std::uint8_t* = 0;
    [[nodiscard]] virtual auto allocateEmotionPairs(std::size_t count) noexcept -> UFeelPair* = 0;
    [[nodiscard]] virtual auto duplicateString(const std::string& value) noexcept -> char* = 0;
};

class SystemWrapperAllocator final : public WrapperAllocator
{
public:

    [[nodiscard]] auto createFrame() noexcept -> UFeelFrame* override
    {
        return new (std::nothrow) UFeelFrame{};
    }

    [[nodiscard]] auto allocateFrameData(
        std::size_t size
    ) noexcept -> std::uint8_t* override
    {
        return static_cast<std::uint8_t*>(std::malloc(size));
    }

    [[nodiscard]] auto allocateEmotionPairs(
        std::size_t count
    ) noexcept -> UFeelPair* override
    {
        return new (std::nothrow) UFeelPair[count]{};
    }

    [[nodiscard]] auto duplicateString(
        const std::string& value
    ) noexcept -> char* override
    {
        return duplicateUtf8(value);
    }
};

inline auto defaultWrapperAllocator() -> WrapperAllocator&
{
    static SystemWrapperAllocator allocator;
    return allocator;
}

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

#endif // UFEELPROCESSOR_HPP
