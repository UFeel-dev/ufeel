#ifndef UFEEL_MODEL_FRAME_HPP
#define UFEEL_MODEL_FRAME_HPP

#include <cstdint>
#include <vector>

namespace ufeel
{

struct FrameSize
{
    int width = 0;
    int height = 0;

    auto operator==(const FrameSize&) const -> bool = default;
};

struct Frame
{
    std::vector<std::uint8_t> data;
    int width = 0;
    int height = 0;
    int channels = 0;
    int stride = 0;
};

} // namespace ufeel

#endif // UFEEL_MODEL_FRAME_HPP
