#include "DataProcessor.hpp"
#include "internal/ErrorHandling.hpp"
#include "internal/UFeelProcessor.hpp" // IWYU pragma: keep
#include "ufeel.h"

#include <cstdint>
#include <cstdlib>

using ufeel::internal::guarded;
using ufeel::internal::guardedVoid;
using ufeel::internal::toUFeelStatus;

namespace
{

void clearSpeechPayload(
    UFeelSpeechToTextResult& result
)
{
    std::free(result.speech);
    result = {};
}

} // namespace

extern "C" {

auto ufeel_update_speech_to_text(
    UFeelProcessor* processor
) -> int32_t
{
    return guarded(
        "ufeel_update_speech_to_text",
        int32_t{UFEEL_STATUS_INTERNAL_ERROR},
        [processor] -> int32_t {
            if (processor == nullptr)
            {
                return UFEEL_STATUS_INVALID_PROCESSOR;
            }

            const auto updateStatus = processor->processor->updateSpeechToText();
            if (updateStatus != DataProcessorStatus::Ok)
            {
                return toUFeelStatus(updateStatus);
            }

            return processor->processor->getSpeechToText().status == SpeechToTextStatus::Valid
                       ? UFEEL_STATUS_OK
                       : UFEEL_STATUS_NO_SPEECH;
        }
    );
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
            processor->processor->toggleSpeechToText(enabled != 0);
            return UFEEL_STATUS_OK;
        }
    );
}

auto ufeel_get_speech_to_text_result(
    UFeelProcessor* processor, UFeelSpeechToTextResult* result
) -> int32_t
{
    return guarded(
        "ufeel_get_speech_to_text_result",
        int32_t{UFEEL_STATUS_INTERNAL_ERROR},
        [processor, result] -> int32_t {
            if (result == nullptr)
            {
                return UFEEL_STATUS_INVALID_ARGUMENT;
            }
            clearSpeechPayload(*result);
            if (processor == nullptr)
            {
                return UFEEL_STATUS_INVALID_PROCESSOR;
            }
            const auto& speech = processor->processor->getSpeechToText();
            result->status = static_cast<int32_t>(speech.status);
            result->speech = processor->allocator->duplicateString(speech.text);
            if (result->speech == nullptr)
            {
                return UFEEL_STATUS_INTERNAL_ERROR;
            }

            return speech.status == SpeechToTextStatus::Valid ? UFEEL_STATUS_OK
                                                              : UFEEL_STATUS_NO_SPEECH;
        }
    );
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
        clearSpeechPayload(*speech);
        delete speech;
    });
}
}
