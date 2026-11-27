#include "SpeechToText/PulseSources.hpp"

#include <functional>
#include <optional>

auto readPulseSources() -> std::optional<PulseSources>
{
    return std::nullopt;
}

auto watchPulseSources() -> std::function<bool()>
{
    return {};
}
