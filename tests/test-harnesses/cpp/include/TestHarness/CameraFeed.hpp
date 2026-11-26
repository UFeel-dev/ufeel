#ifndef TESTHARNESS_CAMERAFEED_HPP
#define TESTHARNESS_CAMERAFEED_HPP

#include "ufeel/Model/Frame.hpp"
#include "ufeel/Native/Processor.hpp"

#include <cstdint>
#include <mutex>
#include <raylib.h>
#include <vector>

class CameraFeed
{
public:

    CameraFeed() = default;
    ~CameraFeed();

    CameraFeed(const CameraFeed&) = delete;
    CameraFeed(CameraFeed&&) = delete;
    auto operator=(const CameraFeed&) -> CameraFeed& = delete;
    auto operator=(CameraFeed&&) -> CameraFeed& = delete;

    void capture(ufeel::Processor& processor);

    void upload();
    void draw(Rectangle bounds) const;

private:

    struct Pixels
    {
        std::vector<std::uint8_t> rgba;
        ufeel::FrameSize size;
    };

    Pixels captured_;

    std::mutex mutex_;
    Pixels pending_;
    bool hasPending_ = false;

    Pixels uploaded_;
    Texture2D texture_{};
};

#endif // TESTHARNESS_CAMERAFEED_HPP
