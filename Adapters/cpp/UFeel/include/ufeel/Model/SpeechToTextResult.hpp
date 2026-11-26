#ifndef UFEEL_MODEL_SPEECHTOTEXTRESULT_HPP
#define UFEEL_MODEL_SPEECHTOTEXTRESULT_HPP

#include "ufeel.h"

#include <cstdint>
#include <string>

namespace ufeel
{

enum class SpeechToTextStatus : std::uint8_t
{
    Disabled = UFEEL_SPEECH_STATUS_DISABLED,
    NoSpeech = UFEEL_SPEECH_STATUS_NO_SPEECH,
    Valid = UFEEL_SPEECH_STATUS_VALID,
};

struct SpeechToTextResult
{
    SpeechToTextStatus status = SpeechToTextStatus::Disabled;
    std::string text;

    [[nodiscard]] auto isValid() const -> bool
    {
        return status == SpeechToTextStatus::Valid;
    }
};

} // namespace ufeel

#endif // UFEEL_MODEL_SPEECHTOTEXTRESULT_HPP
