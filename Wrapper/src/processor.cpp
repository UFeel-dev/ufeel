#include "DataProcessor.hpp"
#include "EyeTracking/EyeTrackingCalibrationModel.hpp"
#include "EyeTracking/EyeTrackingDetector.hpp"
#include "HeartRate/HeartRateSource.hpp"
#include "internal/ErrorHandling.hpp"
#include "internal/FrameConversion.hpp"
#include "internal/UFeelProcessor.hpp"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/imgproc.hpp"
#include "ufeel.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <string>

using ufeel::internal::convertFrame;
using ufeel::internal::FrameCopyPolicy;
using ufeel::internal::guarded;
using ufeel::internal::guardedVoid;
using ufeel::internal::toUFeelStatus;

namespace
{

class DataProcessorBackend final : public ufeel::internal::ProcessorBackend
{
public:

    DataProcessorBackend(
        const std::string& modelDirectory, const std::string& speechModelPath
    )
        : processor_(modelDirectory, speechModelPath)
    {
    }

    auto update(
        std::uint32_t detectionType
    ) -> DataProcessorStatus override
    {
        return processor_.update(detectionType);
    }

    auto updateFromFrame(
        std::uint32_t detectionType, const cv::Mat& frame
    ) -> DataProcessorStatus override
    {
        return processor_.updateFromFrame(detectionType, frame);
    }

    auto updateFrame() -> DataProcessorStatus override
    {
        return processor_.updateFrame();
    }

    auto setFrame(
        const cv::Mat& frame
    ) -> DataProcessorStatus override
    {
        return processor_.setFrame(frame);
    }

    [[nodiscard]] auto getFrame() const -> cv::Mat override
    {
        return processor_.getFrame();
    }

    auto updateEmotions() -> DataProcessorStatus override
    {
        return processor_.updateEmotions();
    }

    auto updateEmotionsFromFrame(
        const cv::Mat& frame
    ) -> DataProcessorStatus override
    {
        return processor_.updateEmotionsFromFrame(frame);
    }

    [[nodiscard]] auto getEmotions() const -> const EmotionResult& override
    {
        return processor_.getEmotions();
    }

    [[nodiscard]] auto getEmotionsFromFrame(
        const cv::Mat& frame
    ) const -> EmotionResult override
    {
        return processor_.getEmotionsFromFrame(frame);
    }

    auto updateEyeTracking() -> DataProcessorStatus override
    {
        return processor_.updateEyeTracking();
    }

    auto updateEyeTrackingFromFrame(
        const cv::Mat& frame
    ) -> DataProcessorStatus override
    {
        return processor_.updateEyeTrackingFromFrame(frame);
    }

    [[nodiscard]] auto getEyeTracking() const -> EyeTrackingResult override
    {
        return processor_.getEyeTracking();
    }

    [[nodiscard]] auto getEyeTrackingFromFrame(
        const cv::Mat& frame
    ) const -> EyeTrackingResult override
    {
        return processor_.getEyeTrackingFromFrame(frame);
    }

    auto updateSpeechToText() -> DataProcessorStatus override
    {
        return processor_.updateSpeechToText();
    }

    [[nodiscard]] auto getSpeechToText() const -> const SpeechToTextResult& override
    {
        return processor_.getSpeechToText();
    }

    void toggleSpeechToText(
        bool enabled
    ) override
    {
        processor_.toggleSpeechToText(enabled);
    }

    auto updateHeartRate() -> DataProcessorStatus override
    {
        return processor_.updateHeartRate();
    }

    [[nodiscard]] auto getHeartRate() const -> HeartRateResult override
    {
        return processor_.getHeartRate();
    }

    void beginEyeTrackingCalibration() override
    {
        processor_.beginEyeTrackingCalibration();
    }

    [[nodiscard]] auto setEyeTrackingCalibrationTarget(
        float x, float y
    ) -> bool override
    {
        return processor_.setEyeTrackingCalibrationTarget(x, y);
    }

    [[nodiscard]] auto finishEyeTrackingCalibration() -> bool override
    {
        return processor_.finishEyeTrackingCalibration();
    }

    void cancelEyeTrackingCalibration() override
    {
        processor_.cancelEyeTrackingCalibration();
    }

    [[nodiscard]] auto getEyeTrackingCalibrationProgress() const -> CalibrationProgress override
    {
        return processor_.getEyeTrackingCalibrationProgress();
    }

    [[nodiscard]] auto getEyeTrackingCalibrationDiagnostics() const
        -> CalibrationDiagnostics override
    {
        return processor_.getEyeTrackingCalibrationDiagnostics();
    }

    [[nodiscard]] auto saveEyeTrackingCalibrationProfile(
        const std::string& path
    ) const -> bool override
    {
        return processor_.saveEyeTrackingCalibrationProfile(path);
    }

    [[nodiscard]] auto loadEyeTrackingCalibrationProfile(
        const std::string& path
    ) -> bool override
    {
        return processor_.loadEyeTrackingCalibrationProfile(path);
    }

private:

    DataProcessor processor_;
};

} // namespace

UFeelProcessor::UFeelProcessor(
    const std::string& modelDirectory, const std::string& speechModelPath
)
    : processor(std::make_unique<DataProcessorBackend>(modelDirectory, speechModelPath))
{
}

namespace
{

auto tightFrameLayout(
    const cv::Mat& frame, int32_t& stride, int32_t& byteCount
) -> bool
{
    if (frame.empty() || frame.cols <= 0 || frame.rows <= 0 || frame.channels() <= 0)
    {
        return false;
    }

    constexpr auto limit = static_cast<size_t>(std::numeric_limits<int32_t>::max());
    const auto width = static_cast<size_t>(frame.cols);
    const auto elementSize = frame.elemSize();
    if (elementSize > limit / width)
    {
        return false;
    }

    const auto rowBytes = width * elementSize;
    const auto height = static_cast<size_t>(frame.rows);

    if (rowBytes > limit || rowBytes > limit / height)
    {
        return false;
    }

    stride = static_cast<int32_t>(rowBytes);
    byteCount = static_cast<int32_t>(rowBytes * height);
    return true;
}

} // namespace

extern "C" {

// TODO: make it possible to choose the frame reader (OpenCV cap.read() by default)

auto ufeel_create(
    const char* utf8ModelDirectory, const char* utf8SpeechModelPath
) -> UFeelProcessor*
{
    if (utf8ModelDirectory == nullptr || utf8ModelDirectory[0] == '\0' ||
        utf8SpeechModelPath == nullptr || utf8SpeechModelPath[0] == '\0')
    {
        return nullptr;
    }

    return guarded(
        "ufeel_create",
        static_cast<UFeelProcessor*>(nullptr),
        [utf8ModelDirectory, utf8SpeechModelPath] -> UFeelProcessor* {
            return new (std::nothrow) UFeelProcessor(utf8ModelDirectory, utf8SpeechModelPath);
        }
    );
}

void ufeel_destroy(
    UFeelProcessor* processor
)
{
    guardedVoid("ufeel_destroy", [processor] -> void { delete processor; });
}

auto ufeel_update(
    UFeelProcessor* processor, int32_t detectionType
) -> int32_t
{
    if (processor == nullptr)
    {
        return UFEEL_STATUS_INVALID_PROCESSOR;
    }

    return guarded(
        "ufeel_update",
        int32_t{UFEEL_STATUS_INTERNAL_ERROR},
        [processor, detectionType] -> int32_t {
            return toUFeelStatus(processor->processor->update(detectionType));
        }
    );
}

auto ufeel_update_from_frame(
    UFeelProcessor* processor, int32_t detectionType, const UFeelFrame* frame
) -> int32_t
{
    return guarded(
        "ufeel_update_from_frame",
        int32_t{UFEEL_STATUS_INTERNAL_ERROR},
        [processor, detectionType, frame] -> int32_t {
            if (processor == nullptr)
            {
                return UFEEL_STATUS_INVALID_PROCESSOR;
            }

            const cv::Mat frameConverted = convertFrame(frame, FrameCopyPolicy::Borrow);
            if (frameConverted.empty())
            {
                return UFEEL_STATUS_INVALID_FRAME;
            }

            return toUFeelStatus(
                processor->processor->updateFromFrame(detectionType, frameConverted)
            );
        }
    );
}

auto ufeel_update_frame(
    UFeelProcessor* processor
) -> int32_t
{
    if (processor == nullptr)
    {
        return UFEEL_STATUS_INVALID_PROCESSOR;
    }

    return guarded(
        "ufeel_update_frame", int32_t{UFEEL_STATUS_INTERNAL_ERROR}, [processor] -> int32_t {
            return toUFeelStatus(processor->processor->updateFrame());
        }
    );
}

auto ufeel_set_frame(
    UFeelProcessor* processor, const UFeelFrame* frame
) -> int32_t
{
    return guarded(
        "ufeel_set_frame", int32_t{UFEEL_STATUS_INTERNAL_ERROR}, [processor, frame] -> int32_t {
            if (processor == nullptr)
            {
                return UFEEL_STATUS_INVALID_PROCESSOR;
            }

            const cv::Mat frameConverted = convertFrame(frame, FrameCopyPolicy::Borrow);
            if (frameConverted.empty())
            {
                return UFEEL_STATUS_INVALID_FRAME;
            }

            return toUFeelStatus(processor->processor->setFrame(frameConverted));
        }
    );
}

auto ufeel_get_frame(
    UFeelProcessor* processor
) -> UFeelFrame*
{
    if (processor == nullptr)
    {
        return nullptr;
    }

    return guarded(
        "ufeel_get_frame", static_cast<UFeelFrame*>(nullptr), [processor] -> UFeelFrame* {
            const cv::Mat frame = processor->processor->getFrame();
            if (frame.empty())
            {
                return static_cast<UFeelFrame*>(nullptr);
            }

            if (frame.cols <= 0 || frame.rows <= 0 || frame.channels() <= 0)
            {
                return static_cast<UFeelFrame*>(nullptr);
            }

            const size_t stride = frame.step1(0) * frame.elemSize1();
            const auto height = static_cast<size_t>(frame.rows);

            const auto width = static_cast<size_t>(frame.cols);
            const auto elementSize = frame.elemSize();

            if (stride == 0 || elementSize > std::numeric_limits<size_t>::max() / width ||
                stride < width * elementSize ||
                stride > std::numeric_limits<size_t>::max() / height ||
                stride > static_cast<size_t>(std::numeric_limits<int32_t>::max()))
            {
                return static_cast<UFeelFrame*>(nullptr);
            }

            const size_t byteCount = stride * height;

            auto output = std::unique_ptr<UFeelFrame>(processor->allocator->createFrame());
            if (!output)
            {
                return static_cast<UFeelFrame*>(nullptr);
            }

            output->data = processor->allocator->allocateFrameData(byteCount);
            if (output->data == nullptr)
            {
                return static_cast<UFeelFrame*>(nullptr);
            }

            std::memcpy(output->data, frame.data, byteCount);

            output->width = frame.cols;
            output->height = frame.rows;
            output->channels = frame.channels();
            output->stride = static_cast<int32_t>(stride);
            output->dataSize = byteCount;

            return output.release();
        }
    );
}

void ufeel_free_frame(
    UFeelFrame* frame
)
{
    guardedVoid("ufeel_free_frame", [frame] -> void {
        if (frame == nullptr)
        {
            return;
        }

        std::free(frame->data);
        frame->data = nullptr;
        delete frame;
    });
}

auto ufeel_get_frame_info(
    UFeelProcessor* processor, UFeelFrame* info
) -> int32_t
{
    if (processor == nullptr)
    {
        return UFEEL_STATUS_INVALID_PROCESSOR;
    }

    if (info == nullptr)
    {
        return UFEEL_STATUS_INVALID_ARGUMENT;
    }

    return guarded(
        "ufeel_get_frame_info",
        static_cast<int32_t>(UFEEL_STATUS_INTERNAL_ERROR),
        [processor, info] -> int32_t {
            const cv::Mat frame = processor->processor->getFrame();

            int32_t stride = 0;
            int32_t byteCount = 0;
            if (!tightFrameLayout(frame, stride, byteCount))
            {
                return UFEEL_STATUS_INVALID_FRAME;
            }

            info->data = nullptr;
            info->width = frame.cols;
            info->height = frame.rows;
            info->channels = frame.channels();
            info->stride = stride;
            info->dataSize = static_cast<uint64_t>(byteCount);

            return UFEEL_STATUS_OK;
        }
    );
}

auto ufeel_copy_frame(
    UFeelProcessor* processor, uint8_t* destination, int32_t destinationSize
) -> int32_t
{
    if (processor == nullptr)
    {
        return UFEEL_STATUS_INVALID_PROCESSOR;
    }

    if (destination == nullptr || destinationSize <= 0)
    {
        return UFEEL_STATUS_INVALID_ARGUMENT;
    }

    return guarded(
        "ufeel_copy_frame",
        static_cast<int32_t>(UFEEL_STATUS_INTERNAL_ERROR),
        [processor, destination, destinationSize] -> int32_t {
            const cv::Mat frame = processor->processor->getFrame();

            int32_t stride = 0;
            int32_t byteCount = 0;
            if (!tightFrameLayout(frame, stride, byteCount))
            {
                return UFEEL_STATUS_INVALID_FRAME;
            }

            if (destinationSize < byteCount)
            {
                return UFEEL_STATUS_INVALID_ARGUMENT;
            }

            const auto rowBytes = static_cast<size_t>(stride);
            for (int row = 0; row < frame.rows; ++row)
            {
                std::memcpy(
                    destination + (static_cast<size_t>(row) * rowBytes), frame.ptr(row), rowBytes
                );
            }

            return UFEEL_STATUS_OK;
        }
    );
}

auto ufeel_copy_frame_bgra(
    UFeelProcessor* processor,
    uint8_t* destination,
    int32_t destinationRowBytes,
    int32_t width,
    int32_t height
) -> int32_t
{
    if (processor == nullptr)
    {
        return UFEEL_STATUS_INVALID_PROCESSOR;
    }

    if (destination == nullptr || width <= 0 || height <= 0 ||
        static_cast<int64_t>(destinationRowBytes) < static_cast<int64_t>(width) * 4)
    {
        return UFEEL_STATUS_INVALID_ARGUMENT;
    }

    return guarded(
        "ufeel_copy_frame_bgra",
        static_cast<int32_t>(UFEEL_STATUS_INTERNAL_ERROR),
        [processor, destination, destinationRowBytes, width, height] -> int32_t {
            const cv::Mat frame = processor->processor->getFrame();

            if (frame.empty() || frame.depth() != CV_8U)
            {
                return UFEEL_STATUS_INVALID_FRAME;
            }

            if (frame.cols != width || frame.rows != height)
            {
                return UFEEL_STATUS_INVALID_ARGUMENT;
            }

            cv::Mat output(
                height, width, CV_8UC4, destination, static_cast<size_t>(destinationRowBytes)
            );

            switch (frame.channels())
            {
            case 1:
                cv::cvtColor(frame, output, cv::COLOR_GRAY2BGRA);
                break;
            case 3:
                cv::cvtColor(frame, output, cv::COLOR_BGR2BGRA);
                break;
            case 4:
                frame.copyTo(output);
                break;
            default:
                return UFEEL_STATUS_INVALID_FRAME;
            }

            if (output.data != destination)
            {
                return UFEEL_STATUS_INTERNAL_ERROR;
            }

            return UFEEL_STATUS_OK;
        }
    );
}

} // extern "C"
