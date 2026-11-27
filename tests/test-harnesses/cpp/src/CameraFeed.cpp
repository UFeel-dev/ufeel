#include "TestHarness/CameraFeed.hpp"

#include "ufeel/Native/Processor.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <raylib.h>
#include <utility>
#include <vector>

namespace
{

constexpr int bytesPerPixel = 4;

void swapRedAndBlue(
    std::vector<std::uint8_t>& pixels
)
{
    for (std::size_t index = 0; index + 2 < pixels.size(); index += bytesPerPixel)
    {
        std::swap(pixels.at(index), pixels.at(index + 2));
    }
}

} // namespace

CameraFeed::~CameraFeed()
{
    if (texture_.id != 0)
    {
        UnloadTexture(texture_);
    }
}

void CameraFeed::capture(
    ufeel::Processor& processor
)
{
    const auto size = processor.frameSize();
    if (!size)
    {
        return;
    }

    const int rowBytes = size->width * bytesPerPixel;
    captured_.rgba.resize(
        static_cast<std::size_t>(rowBytes) * static_cast<std::size_t>(size->height)
    );
    if (!processor.copyFrameBgra(captured_.rgba, rowBytes, *size))
    {
        return;
    }
    captured_.size = *size;
    swapRedAndBlue(captured_.rgba);

    const std::scoped_lock lock(mutex_);
    std::swap(captured_, pending_);
    hasPending_ = true;
}

void CameraFeed::upload()
{
    {
        const std::scoped_lock lock(mutex_);
        if (!hasPending_)
        {
            return;
        }
        std::swap(pending_, uploaded_);
        hasPending_ = false;
    }

    const auto [width, height] = uploaded_.size;
    if (texture_.id != 0 && texture_.width == width && texture_.height == height)
    {
        UpdateTexture(texture_, uploaded_.rgba.data());
        return;
    }

    if (texture_.id != 0)
    {
        UnloadTexture(texture_);
    }
    texture_ = LoadTextureFromImage({
        .data = uploaded_.rgba.data(),
        .width = width,
        .height = height,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
    });
    SetTextureFilter(texture_, TEXTURE_FILTER_BILINEAR);
}

void CameraFeed::draw(
    Rectangle bounds
) const
{
    if (texture_.id == 0)
    {
        return;
    }

    const auto width = static_cast<float>(texture_.width);
    const auto height = static_cast<float>(texture_.height);
    const float scale = std::min(bounds.width / width, bounds.height / height);
    const Rectangle source{.x = 0.0F, .y = 0.0F, .width = width, .height = height};
    const Rectangle destination{
        .x = bounds.x + ((bounds.width - (width * scale)) / 2.0F),
        .y = bounds.y + ((bounds.height - (height * scale)) / 2.0F),
        .width = width * scale,
        .height = height * scale,
    };

    DrawTexturePro(texture_, source, destination, {.x = 0.0F, .y = 0.0F}, 0.0F, WHITE);
}
