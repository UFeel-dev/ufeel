#include "internal/ErrorHandling.hpp"
#include "internal/FrameConversion.hpp"
#include "internal/UFeelProcessor.hpp"
#include "opencv2/core/mat.hpp"
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
using ufeel::internal::guarded;
using ufeel::internal::guardedVoid;

UFeelProcessor::UFeelProcessor(
    const std::string& modelDirectory, const std::string& speechModelPath
)
    : processor(modelDirectory, speechModelPath)
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
    const auto rowBytes = static_cast<size_t>(frame.cols) * frame.elemSize();
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
) -> int32_t // TODO: return a UFeel status instead of a bool if possible
{
    if (processor == nullptr)
    {
        return 0; // TODO: update function to return status, and return
                  // `UFEEL_STATUS_INVALID_PROCESSOR`
    }

    return guarded("ufeel_update", int32_t{0}, [processor, detectionType] -> int32_t {
        return static_cast<int32_t>(processor->processor.update(detectionType));
    });
}

auto ufeel_update_from_frame(
    UFeelProcessor* processor, int32_t detectionType, const UFeelFrame* frame
) -> int32_t
{
    if (processor == nullptr)
    {
        return 0; // TODO: update function to return status, and return
                  // `UFEEL_STATUS_INVALID_PROCESSOR`
    }

    const cv::Mat frameConverted = convertFrame(frame);

    if (frameConverted.empty())
    {
        return 0; // TODO: update function to return status, and return `UFEEL_STATUS_INVALID_FRAME`
    }

    return guarded(
        "ufeel_update_from_frame",
        int32_t{0},
        [processor, detectionType, frameConverted] -> int32_t {
            return static_cast<int32_t>(
                processor->processor.updateFromFrame(detectionType, frameConverted)
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
        return 0; // TODO: update function to return status, and return
                  // `UFEEL_STATUS_INVALID_PROCESSOR`
    }

    return guarded("ufeel_update_frame", int32_t{0}, [processor] -> int32_t {
        return static_cast<int32_t>(processor->processor.updateFrame());
    });
}

auto ufeel_set_frame(
    UFeelProcessor* processor, const UFeelFrame* frame
) -> int32_t
{
    if (processor == nullptr)
    {
        return UFEEL_STATUS_INVALID_PROCESSOR;
    }

    const cv::Mat frameConverted = convertFrame(frame);

    if (frameConverted.empty())
    {
        return UFEEL_STATUS_INVALID_FRAME;
    }
    return guarded("ufeel_set_frame", int32_t{0}, [processor, frameConverted] -> int32_t {
        return static_cast<int32_t>(processor->processor.setFrame(frameConverted));
    });
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
            const cv::Mat frame = processor->processor.getFrame();
            if (frame.empty())
            {
                return static_cast<UFeelFrame*>(nullptr);
            }

            if (frame.cols <= 0 || frame.rows <= 0 || frame.channels() <= 0)
            {
                return static_cast<UFeelFrame*>(nullptr);
            }

            const size_t stride = frame.step[0];
            const auto height = static_cast<size_t>(frame.rows);

            if (stride == 0 || stride < static_cast<size_t>(frame.cols) * frame.elemSize() ||
                stride > std::numeric_limits<size_t>::max() / height ||
                stride > static_cast<size_t>(std::numeric_limits<int32_t>::max()))
            {
                return static_cast<UFeelFrame*>(nullptr);
            }

            const size_t byteCount = stride * height;

            auto output = std::unique_ptr<UFeelFrame>(new (std::nothrow) UFeelFrame{});
            if (!output)
            {
                return static_cast<UFeelFrame*>(nullptr);
            }

            output->data = static_cast<uint8_t*>(std::malloc(byteCount));
            if (output->data == nullptr)
            {
                return static_cast<UFeelFrame*>(nullptr);
            }

            std::memcpy(output->data, frame.data, byteCount);

            output->width = frame.cols;
            output->height = frame.rows;
            output->channels = frame.channels();
            output->stride = static_cast<int32_t>(stride);

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
            const cv::Mat frame = processor->processor.getFrame();

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
            const cv::Mat frame = processor->processor.getFrame();

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

} // extern "C"
