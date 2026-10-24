#include "HeartRate/HeartRateDetector.hpp"
#include "HeartRate/UFeelHeartRateStatus.hpp"

#include "cpr/cpr.h"
#include "nlohmann/json.hpp"

auto HeartRateSensorDetector::process() -> int
{
    cpr::Response r = cpr::Get(cpr::Url{"http://localhost:8000"});

    if (r.status_code == 200)
    {
        try
        {
            nlohmann::json j = nlohmann::json::parse(r.text);

            return j.at("bpm").get<int>();
        }
        catch (const nlohmann::json::parse_error& e)
        {
            return UFeelHeartRateStatus::UFEEL_HEARTRATE_STATUS_JSON_PARSE_ERROR;
        }
        catch (const nlohmann::json::out_of_range& e)
        {
            return UFeelHeartRateStatus::UFEEL_HEARTRATE_STATUS_JSON_PARSE_OUT_OF_RANGE;
        }
    }

    return UFeelHeartRateStatus::UFEEL_HEARTRATE_STATUS_REQUEST_FAILED;
}
