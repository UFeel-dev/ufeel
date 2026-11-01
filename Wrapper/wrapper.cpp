#include "DataProcessor.hpp"
#include "ufeel.h"
#include "ufeel_utils.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <limits>
#include <memory>
#include <new>
#include <string>
#include <utility>

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
) -> int32_t
{
    if (processor == nullptr)
    {
        return UFEEL_STATUS_INVALID_PROCESSOR;
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
        return UFEEL_STATUS_INVALID_PROCESSOR;
    }

    const cv::Mat frameConverted = convertUFeelFrameToMat(frame);

    if (frameConverted.empty())
    {
        return UFEEL_STATUS_INVALID_FRAME;
    }


    return guarded("ufeel_update_from_frame", int32_t{0}, [processor, detectionType, frameConverted] -> int32_t {
        return static_cast<int32_t>(processor->processor.updateFromFrame(detectionType, frameConverted));
    });
}

auto ufeel_update_frame(
    UFeelProcessor* processor
) -> int32_t
{
    if (processor == nullptr)
    {
        return UFEEL_STATUS_INVALID_PROCESSOR;
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

    const cv::Mat frameConverted = convertUFeelFrameToMat(frame);

    if (frameConverted.empty())
    {
        return 0;
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
}