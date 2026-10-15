#include "HeartRate/HeartRateDetector.hpp"

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
}
