#include "HeartRate/HeartRateDetector.hpp"

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

HeartRateSensorDetector::HeartRateSensorDetector() {}

HeartRateSensorDetector::~HeartRateSensorDetector() = default;

auto HeartRateSensorDetector::process() const -> int
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
            return -1;
        }
        catch (const nlohmann::json::out_of_range& e)
        {
            return -1;
        }
    }

    return -1;
}
