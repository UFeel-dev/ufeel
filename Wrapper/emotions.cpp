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

namespace
{
auto convertEmotionsResultToUFeelPair(
    const std::map<std::string, float>& emotions
) -> UFeelPair*
{
    if (emotions.empty() ||
        emotions.size() > static_cast<size_t>(std::numeric_limits<int32_t>::max()))
    {
        return static_cast<UFeelPair*>(nullptr);
    }

    auto output =
        std::unique_ptr<UFeelPair[]>(new (std::nothrow) UFeelPair[emotions.size()]{});

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
}

extern "C" {
auto ufeel_update_emotions(
    UFeelProcessor* processor
) -> int32_t
{
    return guarded("ufeel_update_emotions", int32_t{0}, [processor] -> int32_t {
        if (processor == nullptr)
        {
            return UFEEL_STATUS_INVALID_PROCESSOR;
        }
        return static_cast<int32_t>(processor->processor.updateEmotions());
    });
}


auto ufeel_update_emotions_from_frame(
    UFeelProcessor* processor, const UFeelFrame* frame
) -> int32_t
{
    return guarded("ufeel_update_emotions_from_frame", int32_t{0}, [processor, frame] -> int32_t {
        if (processor == nullptr)
        {
            return UFEEL_STATUS_INVALID_PROCESSOR;
        }

        const cv::Mat frameConverted = convertUFeelFrameToMat(frame);

        if (frameConverted.empty())
        {
            return UFEEL_STATUS_INVALID_FRAME;
        }

        return static_cast<int32_t>(processor->processor.updateEmotionsFromFrame(frameConverted));
    });
}


auto ufeel_get_emotions_result(
    UFeelProcessor* processor, UFeelEmotionsResult* result
) -> int32_t
{
    return guarded(
        "ufeel_get_emotions_result", int32_t{0}, [processor, result] -> int32_t {
            if (result == nullptr || processor == nullptr)
            {
                return UFEEL_STATUS_INVALID_ARGUMENT;
            }

            *result = {};

            const auto emotions = processor->processor.getEmotions();

            result->emotions = convertEmotionsResultToUFeelPair(emotions);
            result->count = emotions.size();
            result->status = UFEEL_EMOTIONS_STATUS_VALID;
            return UFEEL_STATUS_OK;
        }
    );
}

void ufeel_get_emotions_result_from_frame(
    UFeelProcessor* processor, UFeelFrame* frame, void (*handler)(UFeelEmotionsResult* emotions)
)
{
    guardedVoid(
        "ufeel_get_emotions_from_frame", [processor, frame, handler] {
            auto result = std::make_unique<UFeelEmotionsResult>();

            *result = {};

            if (processor == nullptr)
            {
                result->status = UFEEL_STATUS_INVALID_PROCESSOR;
                handler(result.get());
                return;
            }

            const cv::Mat frameConverted = convertUFeelFrameToMat(frame);

            if (frameConverted.empty())
            {
                result->status = UFEEL_STATUS_INVALID_FRAME;
                handler(result.get());
                return;
            }
            const auto emotions = processor->processor.getEmotionsFromFrame(frameConverted);

            result->emotions = convertEmotionsResultToUFeelPair(emotions);
            result->count = emotions.size();
            result->status = UFEEL_EMOTIONS_STATUS_VALID;

            handler(result.get());
        }
    );
}

auto ufeel_create_emotions_result() -> UFeelEmotionsResult*
{
    return guarded("ufeel_create_emotions_result", static_cast<UFeelEmotionsResult*>(nullptr), [] -> UFeelEmotionsResult* {
        auto* result = new UFeelEmotionsResult{};

        *result = {};
        return result;
    });
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
        if (emotions->emotions != nullptr)
        {
            for (int32_t index = 0; index < emotions->count; ++index)
            {
                std::free(emotions->emotions[index].name);
            }
            delete[] emotions->emotions;
        }
        delete[] emotions;
    });
}
}