#ifndef HEARTRATE_UFEELHEARTRATESTATUS_HPP
#define HEARTRATE_UFEELHEARTRATESTATUS_HPP

#include <cstdint>

enum class HeartRateStatus : std::uint8_t
{
    JsonParseError = 0,
    JsonParseOutOfRange = 1,
    RequestFailed = 2,
    Valid = 3,
};

#endif // HEARTRATE_UFEELHEARTRATESTATUS_HPP
