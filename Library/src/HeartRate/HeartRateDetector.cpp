#include "HeartRate/HeartRateDetector.hpp"

<<<<<<< HEAD
#include "HeartRate/UFeelHeartRateStatus.hpp"
#include "cpr/api.h"
#include "cpr/cprtypes.h"
#include "cpr/response.h"
#include "nlohmann/json.hpp"

#include <cstdint>

auto HeartRateSensorDetector::process() -> HeartRateResult
{
    cpr::Response const r = cpr::Get(cpr::Url{"http://localhost:8000"});

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
=======
#include <string>
#include <regex>
#include <cpr/cpr.h>
#include <fstream>

HeartRateSensorDetector::HeartRateSensorDetector() {}

HeartRateSensorDetector::~HeartRateSensorDetector() = default;

auto HeartRateSensorDetector::process(
) const -> int
{
    cpr::Response r = cpr::Get(cpr::Url{"http://localhost:8000"});

    if (r.status_code == 200)
    {
        try 
        {
            std::regex bpm_regex(R"(\"bpm\"\s*:\s*(\d+))");
            std::smatch match;

            if (std::regex_search(r.text, match, bpm_regex)) 
            {
                return std::stoi(match[1].str());
            }
        }
        catch (const std::exception& e) 
        {
            return -1; 
        }
    }
    
    return -1;
>>>>>>> e88a471 (feat: add heartbeat in cpp library)
}
