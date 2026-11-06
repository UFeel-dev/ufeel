#include "HeartRate/SensorHeartRateSource.hpp"

#include "HeartRate/HeartRateSource.hpp"
#include "HeartRate/UFeelHeartRateStatus.hpp"
#include "cpr/api.h"
#include "cpr/cprtypes.h"
#include "cpr/response.h"
#include "cpr/timeout.h"
#include "nlohmann/json.hpp"

#include <chrono>
#include <cstdint>
#include <stop_token>
#include <string>
#include <thread>
#include <utility>

namespace
{

constexpr auto kPollInterval = std::chrono::milliseconds(200);
constexpr auto kRequestTimeout = std::chrono::milliseconds(500);

} // namespace

SensorHeartRateSource::SensorHeartRateSource(
    std::string url
)
    : url_(std::move(url))
    , worker_([this](const std::stop_token& stop) -> void { run(stop); })
{
}

auto SensorHeartRateSource::read() const -> HeartRateResult
{
    return latest_.get();
}

void SensorHeartRateSource::run(
    const std::stop_token& stop
)
{
    while (!stop.stop_requested())
    {
        latest_.set(poll());
        std::this_thread::sleep_for(kPollInterval);
    }
}

auto SensorHeartRateSource::poll() const -> HeartRateResult
{
    cpr::Response const r = cpr::Get(cpr::Url{url_}, cpr::Timeout{kRequestTimeout});

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
        catch (const nlohmann::json::type_error& e)
        {
            result.status = HeartRateStatus::JsonParseError;
        }
    }
    else
    {
        result.status = HeartRateStatus::RequestFailed;
    }

    return result;
}
