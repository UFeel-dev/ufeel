#ifndef FRAMECONVERSION_HPP
#define FRAMECONVERSION_HPP

#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "ufeel.h"

#include <cstddef>
#include <limits>
#include <utility>

namespace ufeel::internal
{

inline auto convertFrame(
    const UFeelFrame* frame
) -> cv::Mat // TODO: add option to avoid the copy for parts that do not need it
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

    const int type = CV_MAKETYPE(CV_8U, frame->channels);

    return cv::Mat(
               frame->height, frame->width, type, frame->data, static_cast<size_t>(frame->stride)
    )
        .clone();
}

} // namespace ufeel::internal

#endif // FRAMECONVERSION_HPP
