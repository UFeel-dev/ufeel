#ifndef INTERNAL_FRAMECONVERSION_HPP
#define INTERNAL_FRAMECONVERSION_HPP

#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "ufeel.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>

namespace ufeel::internal
{

enum class FrameCopyPolicy : std::uint8_t
{
    Borrow,
    Copy,
};

inline auto convertFrame(
    const UFeelFrame* frame, FrameCopyPolicy copyPolicy = FrameCopyPolicy::Copy
) -> cv::Mat
{
    if (frame == nullptr || frame->data == nullptr)
    {
        return {};
    }

    if (frame->width <= 0 || frame->height <= 0 || frame->channels <= 0 ||
        frame->channels > CV_CN_MAX || frame->stride <= 0)
    {
        return {};
    }

    const auto width = static_cast<size_t>(frame->width);
    const auto channels = static_cast<size_t>(frame->channels);

    if (width > std::numeric_limits<size_t>::max() / channels)
    {
        return {};
    }

    const size_t minimumStride = width * channels;

    if (std::cmp_less(frame->stride, minimumStride))
    {
        return {};
    }

    const auto height = static_cast<size_t>(frame->height);
    const auto stride = static_cast<size_t>(frame->stride);
    if (stride > std::numeric_limits<size_t>::max() / height)
    {
        return {};
    }
    const auto dataSize = stride * height;
    if (std::cmp_less(frame->dataSize, dataSize))
    {
        return {};
    }

    const int type = CV_MAKETYPE(CV_8U, frame->channels);

    const cv::Mat converted(frame->height, frame->width, type, frame->data, stride);

    return copyPolicy == FrameCopyPolicy::Copy ? converted.clone() : converted;
}

} // namespace ufeel::internal

#endif // INTERNAL_FRAMECONVERSION_HPP
