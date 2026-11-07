#include "DataProcessor.hpp"
#include "internal/ErrorHandling.hpp"
#include "internal/FrameConversion.hpp"
#include "internal/Memory.hpp"
#include "internal/UFeelProcessor.hpp" // IWYU pragma: keep
#include "opencv2/core/mat.hpp"
#include "ufeel.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>

using ufeel::internal::convertFrame;
using ufeel::internal::duplicateUtf8;
using ufeel::internal::FrameCopyPolicy;
using ufeel::internal::guarded;
using ufeel::internal::guardedVoid;
using ufeel::internal::toUFeelStatus;

namespace
{

void clearEmotionPayload(
    UFeelEmotionsResult& result
)
{
    if (result.emotions != nullptr)
    {
        for (int32_t index = 0; index < result.count; ++index)
        {
            std::free(result.emotions[index].name);
        }
        delete[] result.emotions;
    }

    result = {};
}

struct EmotionResultDeleter
{
    void operator()(
        UFeelEmotionsResult* result
    ) const
    {
        if (result != nullptr)
        {
            clearEmotionPayload(*result);
            delete result;
        }
    }
};

auto convertEmotionsResultToUFeelPair(
    const std::map<std::string, float>& emotions
) -> UFeelPair*
{
    if (emotions.empty() ||
        emotions.size() > static_cast<size_t>(std::numeric_limits<int32_t>::max()))
    {
        return static_cast<UFeelPair*>(nullptr);
    }

    auto output = std::unique_ptr<UFeelPair[]>(new (std::nothrow) UFeelPair[emotions.size()]{});

    if (!output)
    {
        return static_cast<UFeelPair*>(nullptr);
    }

    int32_t index = 0;
    for (const auto& [name, value] : emotions)
    {
        output[index].name = duplicateUtf8(name);
        if (output[index].name == nullptr)
        {
            for (int32_t allocated = 0; allocated < index; ++allocated)
            {
                std::free(output[allocated].name);
            }

            return static_cast<UFeelPair*>(nullptr);
        }

        output[index].value = static_cast<float>(value);
        ++index;
    }

    return output.release();
}

} // namespace

extern "C" {

auto ufeel_update_emotions(
    UFeelProcessor* processor
) -> int32_t
{
    return guarded(
        "ufeel_update_emotions", int32_t{UFEEL_STATUS_INTERNAL_ERROR}, [processor] -> int32_t {
            if (processor == nullptr)
            {
                return UFEEL_STATUS_INVALID_PROCESSOR;
            }

            const auto updateStatus = processor->processor.updateEmotions();
            if (updateStatus != DataProcessorStatus::Ok)
            {
                return toUFeelStatus(updateStatus);
            }

            return processor->processor.getEmotions().status == EmotionStatus::Valid
                       ? UFEEL_STATUS_OK
                       : UFEEL_STATUS_NO_EMOTIONS;
        }
    );
}

auto ufeel_update_emotions_from_frame(
    UFeelProcessor* processor, const UFeelFrame* frame
) -> int32_t
{
    return guarded(
        "ufeel_update_emotions_from_frame",
        int32_t{UFEEL_STATUS_INTERNAL_ERROR},
        [processor, frame] -> int32_t {
            if (processor == nullptr)
            {
                return UFEEL_STATUS_INVALID_PROCESSOR;
            }

            const cv::Mat frameConverted = convertFrame(frame, FrameCopyPolicy::Borrow);

            if (frameConverted.empty())
            {
                return UFEEL_STATUS_INVALID_FRAME;
            }

            const auto updateStatus = processor->processor.updateEmotionsFromFrame(frameConverted);
            if (updateStatus != DataProcessorStatus::Ok)
            {
                return toUFeelStatus(updateStatus);
            }

            return processor->processor.getEmotions().status == EmotionStatus::Valid
                       ? UFEEL_STATUS_OK
                       : UFEEL_STATUS_NO_EMOTIONS;
        }
    );
}

auto ufeel_get_emotions_result(
    UFeelProcessor* processor, UFeelEmotionsResult* result
) -> int32_t
{
    return guarded(
        "ufeel_get_emotions_result",
        int32_t{UFEEL_STATUS_INTERNAL_ERROR},
        [processor, result] -> int32_t {
            if (result == nullptr)
            {
                return UFEEL_STATUS_INVALID_ARGUMENT;
            }

            clearEmotionPayload(*result);
            if (processor == nullptr)
            {
                return UFEEL_STATUS_INVALID_PROCESSOR;
            }

            const auto& emotionResult = processor->processor.getEmotions();

            result->emotions = convertEmotionsResultToUFeelPair(emotionResult.emotions);
            if (!emotionResult.emotions.empty() && result->emotions == nullptr)
            {
                return UFEEL_STATUS_INTERNAL_ERROR;
            }

            result->count = static_cast<int32_t>(emotionResult.emotions.size());
            result->status = static_cast<int32_t>(emotionResult.status);
            if (emotionResult.status == EmotionStatus::Valid)
            {
                return UFEEL_STATUS_OK;
            }

            return emotionResult.status == EmotionStatus::FrameUnavailable
                       ? UFEEL_STATUS_INVALID_FRAME
                       : UFEEL_STATUS_NO_EMOTIONS;
        }
    );
}

void ufeel_get_emotions_result_from_frame(
    UFeelProcessor* processor, UFeelFrame* frame, void (*handler)(UFeelEmotionsResult* emotions)
)
{
    guardedVoid("ufeel_get_emotions_result_from_frame", [processor, frame, handler] -> void {
        if (handler == nullptr)
        {
            throw std::invalid_argument("emotion result handler must not be null");
        }

        auto result =
            std::unique_ptr<UFeelEmotionsResult, EmotionResultDeleter>(new UFeelEmotionsResult{});

        if (processor == nullptr)
        {
            result->status = UFEEL_EMOTIONS_STATUS_INVALID_PROCESSOR;
            handler(result.get());
            return;
        }

        const cv::Mat frameConverted = convertFrame(frame, FrameCopyPolicy::Borrow);

        if (frameConverted.empty())
        {
            result->status = UFEEL_EMOTIONS_STATUS_FRAME_UNAVAILABLE;
            handler(result.get());
            return;
        }
        const auto emotionResult = processor->processor.getEmotionsFromFrame(frameConverted);

        result->emotions = convertEmotionsResultToUFeelPair(emotionResult.emotions);
        if (!emotionResult.emotions.empty() && result->emotions == nullptr)
        {
            result->status = UFEEL_EMOTIONS_STATUS_INTERNAL_ERROR;
            handler(result.get());
            return;
        }

        result->count = static_cast<int32_t>(emotionResult.emotions.size());
        result->status = static_cast<int32_t>(emotionResult.status);

        handler(result.get());
    });
}

auto ufeel_create_emotions_result() -> UFeelEmotionsResult*
{
    return guarded(
        "ufeel_create_emotions_result",
        static_cast<UFeelEmotionsResult*>(nullptr),
        [] -> UFeelEmotionsResult* {
            auto* result = new UFeelEmotionsResult{};

            *result = {};
            return result;
        }
    );
}

void ufeel_free_emotions_result(
    UFeelEmotionsResult* emotions
)
{
    guardedVoid("ufeel_free_emotions_result", [emotions] -> void {
        if (emotions == nullptr)
        {
            return;
        }
        clearEmotionPayload(*emotions);
        delete emotions;
    });
}
}
