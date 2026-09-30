#include "DataProcessor.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

#include <cstddef>
#include <cstdlib>
#include <iomanip>
#include <ios>
#include <map>
#include <sstream>
#include <string>
#include <string.h>
#include <utility>

extern "C" {
// WRAPPER API
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

struct UFeelPair
{
        const char* key;
        float value;
};

struct UFeelBoolPair
{
        const char* key;
        bool value;
};

__attribute__((visibility("default"))) auto ufeel_get_emotions(
    void* processor, size_t* size
) -> UFeelPair*
{
    if (processor == nullptr)
    {
        return nullptr;
    }

    auto map = static_cast<DataProcessor*>(processor)->processEmotion();

    *size = map.size();

    auto* emotions = new UFeelPair[*size];

    int i = 0;
    for (auto& [k, v] : map)
    {
        emotions[i].key = strdup(k.c_str());
        emotions[i].value = v;
        i++;
    }

    return emotions;
}

__attribute__((visibility("default"))) void ufeel_free_emotions(
    UFeelPair* emotions, size_t size
)
{
    if (emotions == nullptr)
    {
        return;
    }

    for (size_t i = 0; i < size; i++)
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

    static_cast<DataProcessor*>(processor)->calibrateEyeTracking();
}

__attribute__((visibility("default"))) auto ufeel_get_directions(
    void* processor, size_t* size
) -> UFeelBoolPair*
{
    if (processor == nullptr)
    {
        return nullptr;
    }

    auto directions = static_cast<DataProcessor*>(processor)->processEyeTracking();
    *size = directions.size();
    auto* out = new UFeelBoolPair[*size];

    int i = 0;
    for (auto& [k, v] : directions)
    {
        out[i].key = strdup(k.c_str());
        out[i].value = v;
        i++;
    }

    return out;
}

__attribute__((visibility("default"))) void ufeel_free_directions(
    UFeelBoolPair* directions, int size
)
{
    for (int i = 0; i < size; i++)
    {
        free((void*)directions[i].key);
    }

    delete[] directions;
}

__attribute__((visibility("default"))) void ufeel_toggle_speech(
    void* processor, bool state
)
{
    if (processor == nullptr)
    {
        return;
    }

    static_cast<DataProcessor*>(processor)->toggleSpeechToText(state);
}

__attribute__((visibility("default"))) auto ufeel_get_speech(
    void* processor
) -> const char*
{
    if (processor == nullptr)
    {
        return nullptr;
    }

    std::string const text = static_cast<DataProcessor*>(processor)->processSpeechToText();

    return strdup(text.c_str());
}

__attribute__((visibility("default"))) void ufeel_free_speech(
    char* speech
)
{
    if (speech == nullptr)
    {
        return;
    }

    free(speech);
}

// WRAPPER DEBUG
__attribute__((visibility("default"))) auto ufeel_debug_get_frame(
    void* processor
) -> void*
{
    if (processor == nullptr)
    {
        return nullptr;
    }

    return new cv::Mat(static_cast<DataProcessor*>(processor)->getFrame());
}

__attribute__((visibility("default"))) void ufeel_debug_destroy_frame(
    void* frame
)
{
    delete static_cast<cv::Mat*>(frame);
}

__attribute__((visibility("default"))) void ufeel_debug_show_emotions(
    void* frame, UFeelPair* arr, int size
)
{
    if (frame == nullptr)
    {
        return;
    }

    cv::Mat& cvFrame = *static_cast<cv::Mat*>(frame);
    int y = 30;
    int const lineHeight = 25;

    for (int i = 0; i < size; i++)
    {
        std::ostringstream oss;
        oss << arr[i].key << ": " << std::fixed << std::setprecision(3) << arr[i].value;

        cv::putText(
            cvFrame,
            oss.str(),
            cv::Point(20, y),
            cv::FONT_HERSHEY_SIMPLEX,
            0.6,
            cv::Scalar(255, 255, 255),
            2
        );

        y += lineHeight;
    }

    cv::imshow("Emotion Detection", cvFrame);
}

__attribute__((visibility("default"))) void ufeel_debug_show_directions(
    void* frame, UFeelBoolPair* arr, int size
)
{
    cv::Mat& cvFrame = *static_cast<cv::Mat*>(frame);
    // for (int i = 0; i < p->first_size; i++)
    // {
    //     cv::Point2d point = cv::Point2d(p->first[i].x, p->first[i].y);
    //     cv::circle(cvFrame, point, 2, cv::Scalar(0, 0, 255), -1); // BGR → RED
    // }

    // for (int i = 0; i < p->second_size; i++)
    // {
    //     cv::Point2d point = cv::Point2d(p->second[i].x, p->second[i].y);
    //     cv::circle(cvFrame, point, 2, cv::Scalar(255, 0, 0), -1); // BGR → BLUE
    // }

    int y = 30;
    int const lineHeight = 25;

    for (int i = 0; i < size; i++)
    {
        std::ostringstream oss;
        oss << arr[i].key << ": " << arr[i].value;

        cv::putText(
            cvFrame,
            oss.str(),
            cv::Point(20, y),
            cv::FONT_HERSHEY_SIMPLEX,
            0.6,
            cv::Scalar(255, 255, 255),
            2
        );

        y += lineHeight;
    }

    cv::imshow("Eye Tracking Detection", cvFrame);
}

__attribute__((visibility("default"))) void ufeel_debug_show_speech(
    void* frame, char* speech
)
{
    if (frame == nullptr)
    {
        return;
    }

    cv::Mat& cvFrame = *static_cast<cv::Mat*>(frame);

    std::ostringstream oss;
    oss << "Current speech: " << speech;

    cv::putText(
        cvFrame,
        oss.str(),
        cv::Point(20, 30),
        cv::FONT_HERSHEY_SIMPLEX,
        0.6,
        cv::Scalar(255, 255, 255),
        2
    );

    cv::imshow("Debug UFeel", cvFrame);
}

__attribute__((visibility("default"))) void ufeel_debug_show_frame(
    void* frame
)
{
    if (frame == nullptr)
    {
        return;
    }

    cv::Mat const& cvFrame = *static_cast<cv::Mat*>(frame);
    cv::imshow("Debug UFeel", cvFrame);
}

__attribute__((visibility("default"))) auto ufeel_debug_wait_key(
    int delay
) -> int
{
    return cv::waitKey(delay);
}
}
