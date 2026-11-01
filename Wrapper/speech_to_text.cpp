#include "DataProcessor.hpp"
#include "ufeel.h"
#include "ufeel_utils.h"

#include <cstdint>
#include <cstdlib>
#include <new>

extern "C" {
auto ufeel_update_speech_to_text(
    UFeelProcessor* processor
) -> int32_t
{
    return guarded("ufeel_update_speech_to_text", int32_t{0}, [processor] -> int32_t {
        if (processor == nullptr)
        {
            return 0;
        }
        return static_cast<int32_t>(processor->processor.updateSpeechToText());
    });
}

auto ufeel_set_speech_to_text_enabled(
    UFeelProcessor* processor, int32_t enabled
) -> int32_t
{
    return guarded(
        "ufeel_set_speech_to_text_enabled",
        int32_t{UFEEL_STATUS_INTERNAL_ERROR},
        [processor, enabled] -> UFeelStatus {
            if (processor == nullptr)
            {
                return UFEEL_STATUS_INVALID_PROCESSOR;
            }
            processor->processor.toggleSpeechToText(enabled != 0);
            return UFEEL_STATUS_OK;
        }
    );
}

auto ufeel_get_speech_to_text_result(
    UFeelProcessor* processor, UFeelSpeechToTextResult* result
) -> int32_t
{
    return guarded("ufeel_get_speech_to_text_result", int32_t{0}, [processor, result] -> int32_t {
        if (result == nullptr || processor == nullptr)
        {
            return UFEEL_STATUS_INVALID_ARGUMENT;
        }
        *result = {};
        result->status = UFEEL_STATUS_OK;
        result->speech = duplicateUtf8(processor->processor.getSpeechToText());
        return result->status;
    });
}

auto ufeel_create_speech_to_text_result() -> UFeelSpeechToTextResult*
{
    return guarded(
        "ufeel_create_speech_to_text_result",
        static_cast<UFeelSpeechToTextResult*>(nullptr),
        [] -> UFeelSpeechToTextResult* {
            auto* result = new UFeelSpeechToTextResult{};
            *result = {};

            return result;
        }
    );
}

void ufeel_free_speech_to_text_result(
    UFeelSpeechToTextResult* speech
)
{
    guardedVoid("ufeel_free_speech_to_text_result", [speech] -> void {
        if (speech == nullptr)
        {
            return;
        }
        if (speech->speech != nullptr)
        {
            std::free(speech->speech);
        }
        delete speech;
    });
}
}
