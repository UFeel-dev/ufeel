#include "ufeel.h"

#include "DataProcessor.hpp"

#include "opencv2/core/mat.hpp"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

extern "C"
{

__attribute__((visibility("default"))) auto ufeel_create() -> void*
{
    return new DataProcessor();
}

__attribute__((visibility("default"))) void ufeel_destroy(
    void* processor
)
{
    delete static_cast<DataProcessor*>(processor);
}

__attribute__((visibility("default"))) auto ufeel_update(
    void* processor
) -> int32_t
{
    if (processor == nullptr)
    {
        return 0;
    }

    return static_cast<DataProcessor*>(processor)->update()
        ? 1
        : 0;
}

__attribute__((visibility("default"))) auto ufeel_get_frame(
    void* processor
) -> UFeelFrame*
{
    if (processor == nullptr)
    {
        return nullptr;
    }

    cv::Mat const frame =
        static_cast<DataProcessor*>(processor)->getFrame();

    if (frame.empty())
    {
        return nullptr;
    }

    if (frame.type() != CV_8UC3)
    {
        return nullptr;
    }

    auto* out = new UFeelFrame();

    out->width =
        static_cast<uint32_t>(frame.cols);

    out->height =
        static_cast<uint32_t>(frame.rows);

    out->stride =
        static_cast<uint32_t>(
            frame.cols * 3);

    size_t const size =
        static_cast<size_t>(out->stride) *
        out->height;

    out->data =
        new uint8_t[size];

    for (uint32_t y = 0; y < out->height; y++)
    {
        std::memcpy(
            out->data +
                (static_cast<size_t>(y) * out->stride),
            frame.ptr(y),
            out->stride
        );
    }

    return out;
}

__attribute__((visibility("default"))) void ufeel_free_frame(
    UFeelFrame* frame
)
{
    if (frame == nullptr)
    {
        return;
    }

    delete[] frame->data;
    delete frame;
}

__attribute__((visibility("default"))) auto ufeel_get_emotions(
    void* processor,
    uint32_t* size
) -> UFeelPair*
{
    if (processor == nullptr || size == nullptr)
    {
        return nullptr;
    }

    auto const& emotions =
        static_cast<DataProcessor*>(processor)
            ->getEmotions();

    *size =
        static_cast<uint32_t>(emotions.size());

    if (*size == 0)
    {
        return nullptr;
    }

    auto* out =
        new UFeelPair[*size];

    uint32_t i = 0;

    for (auto const& [key, value] : emotions)
    {
        out[i].key = strdup(key.c_str());
        out[i].value = value;
        i++;
    }

    return out;
}

__attribute__((visibility("default"))) void ufeel_free_emotions(
    UFeelPair* emotions,
    uint32_t size
)
{
    if (emotions == nullptr)
    {
        return;
    }

    for (uint32_t i = 0; i < size; i++)
    {
        free((void*)emotions[i].key);
    }

    delete[] emotions;
}

__attribute__((visibility("default"))) void ufeel_calibrate_directions(
    void* processor
)
{
    if (processor == nullptr)
    {
        return;
    }

    static_cast<DataProcessor*>(processor)
        ->calibrateEyeTracking();
}

__attribute__((visibility("default"))) auto ufeel_get_directions(
    void* processor,
    uint32_t* size
) -> UFeelBoolPair*
{
    if (processor == nullptr || size == nullptr)
    {
        return nullptr;
    }

    auto const& directions =
        static_cast<DataProcessor*>(processor)
            ->getDirections();

    *size =
        static_cast<uint32_t>(directions.size());

    if (*size == 0)
    {
        return nullptr;
    }

    auto* out =
        new UFeelBoolPair[*size];

    uint32_t i = 0;

    for (auto const& [key, value] : directions)
    {
        out[i].key = strdup(key.c_str());
        out[i].value = value ? 1 : 0;
        i++;
    }

    return out;
}

__attribute__((visibility("default"))) void ufeel_free_directions(
    UFeelBoolPair* directions,
    uint32_t size
)
{
    if (directions == nullptr)
    {
        return;
    }

    for (uint32_t i = 0; i < size; i++)
    {
        free((void*)directions[i].key);
    }

    delete[] directions;
}

__attribute__((visibility("default"))) void ufeel_toggle_speech(
    void* processor,
    uint8_t state
)
{
    if (processor == nullptr)
    {
        return;
    }

    static_cast<DataProcessor*>(processor)
        ->toggleSpeechToText(state != 0);
}

__attribute__((visibility("default"))) auto ufeel_get_speech(
    void* processor
) -> char*
{
    if (processor == nullptr)
    {
        return nullptr;
    }

    std::string const& speech =
        static_cast<DataProcessor*>(processor)
            ->getSpeech();

    return strdup(speech.c_str());
}

__attribute__((visibility("default"))) void ufeel_free_speech(
    char* speech
)
{
    free(speech);
}

}
