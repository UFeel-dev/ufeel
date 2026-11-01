#include "DataProcessor.hpp"
#include "ufeel_utils.h"
#include "ufeel.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <limits>
#include <memory>
#include <new>
#include <string>
#include <utility>

auto duplicateUtf8(
    const std::string& value
) -> char*
{
    auto* result = static_cast<char*>(std::malloc(value.size() + 1));
    if (result == nullptr)
    {
        return nullptr;
    }

    std::memcpy(result, value.c_str(), value.size() + 1);
    return result;
}

// TODO: Do we need to make a deep copy here to prevent any issue because we use the UFeelFrame data ?
auto convertUFeelFrameToMat(
    const UFeelFrame* frame
) -> cv::Mat
{
    if (frame == nullptr || frame->data == nullptr)
    {
        return {};
    }

    if (frame->width <= 0 ||
        frame->height <= 0 ||
        frame->channels <= 0 ||
        frame->stride <= 0)
    {
        return {};
    }

    const size_t minStride =
        static_cast<size_t>(frame->width) *
        static_cast<size_t>(frame->channels);

    if (static_cast<size_t>(frame->stride) < minStride)
    {
        return {};
    }

    const int type = CV_MAKETYPE(CV_8U, frame->channels);

    return cv::Mat(
        frame->height,
        frame->width,
        type,
        frame->data,
        static_cast<size_t>(frame->stride)
    );
}
