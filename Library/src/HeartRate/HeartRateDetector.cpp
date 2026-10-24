#include "HeartRate/HeartRateDetector.hpp"

#include "HeartRate/UFeelHeartRateStatus.hpp"
#include "cpr/api.h"
#include "cpr/cprtypes.h"
#include "cpr/response.h"
#include "nlohmann/json.hpp"

#include <cstdint>

auto HeartRateSensorDetector::process() -> HeartRateResult
{
    cpr::Response r = cpr::Get(cpr::Url{"http://localhost:8000"});

    HeartRateResult result = {.bpm = 0, .status = HeartRateStatus::Valid};

    if (r.status_code == 200)
    {
        try
        {
            nlohmann::json j = nlohmann::json::parse(r.text);

            result.bpm = j.at("bpm").get<std::uint32_t>();
        }
        catch (const nlohmann::json::parse_error& e)
        {
            result.status = HeartRateStatus::JsonParseError;
        }
        catch (const nlohmann::json::out_of_range& e)
        {
            result.status = HeartRateStatus::JsonParseOutOfRange;
        }
    }
    else
    {
        result.status = HeartRateStatus::RequestFailed;
    }

    return result;
}
