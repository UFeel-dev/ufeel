#ifndef UFEELPROCESSOR_HPP
#define UFEELPROCESSOR_HPP

#include "DataProcessor.hpp"
#include "ufeel.h"

#include <string>

namespace ufeel::internal
{

constexpr auto toUFeelStatus(
    DataProcessorStatus status
) noexcept -> UFeelStatus
{
    switch (status)
    {
    case DataProcessorStatus::Ok:
        return UFEEL_STATUS_OK;
    case DataProcessorStatus::InvalidFrame:
    case DataProcessorStatus::FrameUnavailable:
        return UFEEL_STATUS_INVALID_FRAME;
    case DataProcessorStatus::InvalidDetectionType:
        return UFEEL_STATUS_INVALID_ARGUMENT;
    }

    return UFEEL_STATUS_INTERNAL_ERROR;
}

} // namespace ufeel::internal

struct UFeelProcessor
{
    DataProcessor processor;

    UFeelProcessor(const std::string& modelDirectory, const std::string& speechModelPath);
};

#endif // UFEELPROCESSOR_HPP
