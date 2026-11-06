#include "DataProcessor.hpp"
#include "HeartRate/UFeelHeartRateStatus.hpp"
#include "internal/ErrorHandling.hpp"
#include "internal/UFeelProcessor.hpp" // IWYU pragma: keep
#include "ufeel.h"

#include <cstdint>
#include <new>

using ufeel::internal::guarded;
using ufeel::internal::guardedVoid;
using ufeel::internal::toUFeelStatus;

extern "C" {

auto ufeel_update_heart_rate(
    UFeelProcessor* processor
) -> int32_t
{
    return guarded(
        "ufeel_update_heart_rate", int32_t{UFEEL_STATUS_INTERNAL_ERROR}, [processor] -> int32_t {
            if (processor == nullptr)
            {
                return UFEEL_STATUS_INVALID_PROCESSOR;
            }

            const auto updateStatus = processor->processor.updateHeartRate();
            if (updateStatus != DataProcessorStatus::Ok)
            {
                return toUFeelStatus(updateStatus);
            }

            return processor->processor.getHeartRate().status == HeartRateStatus::Valid
                       ? UFEEL_STATUS_OK
                       : UFEEL_STATUS_NO_HEARTRATE;
        }
    );
}

auto ufeel_get_heart_rate_result(
    UFeelProcessor* processor, UFeelHeartRateResult* result
) -> int32_t
{
    return guarded(
        "ufeel_get_heart_rate_result",
        int32_t{UFEEL_STATUS_INTERNAL_ERROR},
        [processor, result] -> UFeelStatus {
            if (processor == nullptr)
            {
                return UFEEL_STATUS_INVALID_PROCESSOR;
            }
            if (result == nullptr)
            {
                return UFEEL_STATUS_INVALID_ARGUMENT;
            }

            *result = {};
            const auto heartrate = processor->processor.getHeartRate();

            result->status = static_cast<int32_t>(heartrate.status);
            result->bpm = heartrate.bpm;

            return heartrate.status == HeartRateStatus::Valid ? UFEEL_STATUS_OK
                                                              : UFEEL_STATUS_NO_HEARTRATE;
        }
    );
}

auto ufeel_create_heart_rate_result() -> UFeelHeartRateResult*
{
    return guarded(
        "ufeel_create_heart_rate_result",
        static_cast<UFeelHeartRateResult*>(nullptr),
        [] -> UFeelHeartRateResult* {
            auto* result = new UFeelHeartRateResult{};
            *result = {};

            return result;
        }
    );
}

void ufeel_free_heart_rate_result(
    UFeelHeartRateResult* heartrate
)
{
    guardedVoid("ufeel_free_heart_rate_result", [heartrate] -> void {
        if (heartrate == nullptr)
        {
            return;
        }

        delete heartrate;
    });
}
}
