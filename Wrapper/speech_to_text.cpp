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

auto ufeel_get_speech_to_text(
    UFeelProcessor* processor, UFeelSpeechResult* result
) -> int32_t
{
    return guarded("ufeel_get_speech_to_text", int32_t{0}, [processor, result] -> int32_t {
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

auto ufeel_create_speech_to_text() -> UFeelSpeechResult*
{
    return guarded("ufeel_get_speech_to_text", static_cast<UFeelSpeechResult*>(nullptr), [] -> UFeelSpeechResult* {
        auto* result = new UFeelSpeechResult{};
        result = {};

        return result;
    });
}

void ufeel_free_speech_to_text(
    UFeelSpeechResult* speech
)
{
    guardedVoid("ufeel_free_string", [speech] -> void {
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