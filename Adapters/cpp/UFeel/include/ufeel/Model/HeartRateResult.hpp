#ifndef UFEEL_MODEL_HEARTRATERESULT_HPP
#define UFEEL_MODEL_HEARTRATERESULT_HPP

#include "ufeel.h"

#include <cstdint>

namespace ufeel
{

enum class HeartRateStatus : std::uint8_t
{
    JsonParseError = UFEEL_HEARTRATE_STATUS_JSON_PARSE_ERROR,
    JsonParseOutOfRange = UFEEL_HEARTRATE_STATUS_JSON_PARSE_OUT_OF_RANGE,
    RequestFailed = UFEEL_HEARTRATE_STATUS_REQUEST_FAILED,
    Valid = UFEEL_HEARTRATE_STATUS_VALID,
    NoData = UFEEL_HEARTRATE_STATUS_NO_DATA,
};

struct HeartRateResult
{
    HeartRateStatus status = HeartRateStatus::NoData;
    std::uint32_t bpm = 0;

    [[nodiscard]] auto isValid() const -> bool
    {
        return status == HeartRateStatus::Valid;
    }
};

} // namespace ufeel

#endif // UFEEL_MODEL_HEARTRATERESULT_HPP
