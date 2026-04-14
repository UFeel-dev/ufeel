/*
** EPITECH PROJECT, 2026
** lib_ufeel
** File description:
** wrapper
*/

#include "DataProcessor.hpp"

extern "C"
{
    // WRAPPER API
    __attribute__((visibility("default")))
    void *ufeel_create()
    {
        return new DataProcessor(false);
    }

    __attribute__((visibility("default")))
    void ufeel_destroy(void *processor)
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

    __attribute__((visibility("default")))
    UFeelPair *ufeel_get_emotions(void *processor, int *size)
    {
        if (!processor)
            return nullptr;

        auto map = static_cast<DataProcessor*>(processor)->processEmotion();

        *size = map.size();

        UFeelPair *emotions = new UFeelPair[*size];

        int i = 0;
        for (auto &[k, v] : map)
        {
            emotions[i].key = strdup(k.c_str());
            emotions[i].value = v;
            i++;
        }

        return emotions;
    }

    __attribute__((visibility("default")))
    void ufeel_free_emotions(UFeelPair *emotions, int size)
    {
        if (!emotions)
            return;

        for (int i = 0; i < size; i++)
            free((void*)emotions[i].key);

        delete[] emotions;
    }

    __attribute__((visibility("default")))
    UFeelBoolPair *ufeel_get_directions(void *processor, int *size)
    {
        if (!processor)
            return nullptr;

        auto directions = static_cast<DataProcessor*>(processor)->processEyeTracking();
        *size = directions.size();
        UFeelBoolPair* out = new UFeelBoolPair[*size];

        int i = 0;
        for (auto& [k,v] : directions)
        {
            out[i].key = strdup(k.c_str());
            out[i].value = v;
            i++;
        }

        return out;
        // std::map<std::string, bool>
        // auto pair = static_cast<DataProcessor*>(processor)->processEyeTracking();

        // std::cout << "Finish process" << std::endl;

        // auto& [left, right] = pair;

        // for (const auto& p : left)
        // {
        //     std::cout << "L: " << p.x << ", " << p.y << std::endl;
        // }

        // for (const auto& p : right)
        // {
        //     std::cout << "R: " << p.x << ", " << p.y << std::endl;
        // }

        // UFeelPointsPair *out = new UFeelPointsPair{};

        // out->first_size = pair.first.size();
        // out->first = out->first_size ? new UFeelPoint2f[out->first_size] : nullptr;

        // std::cout << "start loop" << std::endl;

        // for (size_t i = 0; i < pair.first.size(); i++)
        // {
        //     out->first[i].x = pair.first[i].x;
        //     out->first[i].y = pair.first[i].y;
        // }

        // out->second_size = pair.second.size();
        // out->second = out->second_size ? new UFeelPoint2f[out->second_size] : nullptr;

        // for (size_t i = 0; i < pair.second.size(); i++)
        // {
        //     out->second[i].x = pair.second[i].x;
        //     out->second[i].y = pair.second[i].y;
        // }

        // std::cout << "return ufeel_get_directions" << std::endl;
        // return out;
    }

    __attribute__((visibility("default")))
    void ufeel_free_directions(UFeelBoolPair *directions, int size)
    {
        for (int i = 0; i < size; i++)
            free((void*)directions[i].key);

        delete[] directions;
    }

    __attribute__((visibility("default")))
    const char *ufeel_get_speech(void *processor)
    {
        if (!processor)
            return nullptr;

        std::string text = static_cast<DataProcessor*>(processor)->processSpeechToText();

        return strdup(text.c_str());
    }

    __attribute__((visibility("default")))
    void ufeel_free_speech(char *speech)
    {
        if (!speech) return;

        free(speech);
    }

    // WRAPPER DEBUG
    __attribute__((visibility("default")))
    void *ufeel_debug_get_frame(void *processor)
    {
        if (!processor)
            return nullptr;

        return new cv::Mat(
            static_cast<DataProcessor*>(processor)->getFrame()
        );
    }

    __attribute__((visibility("default")))
    void ufeel_debug_destroy_frame(void* frame)
    {
        delete static_cast<cv::Mat*>(frame);
    }

    __attribute__((visibility("default")))
    void ufeel_debug_show_emotions(void *frame, UFeelPair *arr, int size)
    {
        if (!frame)
            return;

        cv::Mat &cvFrame = *static_cast<cv::Mat*>(frame);
        int y = 30;
        int lineHeight = 25;

        for (int i = 0; i < size; i++)
        {
            std::ostringstream oss;
            oss << arr[i].key << ": " << std::fixed << std::setprecision(3) << arr[i].value;

            cv::putText(cvFrame, oss.str(), cv::Point(20, y),
                        cv::FONT_HERSHEY_SIMPLEX, 0.6,
                        cv::Scalar(255, 255, 255), 2);

            y += lineHeight;
        }

        cv::imshow("Emotion Detection", cvFrame);
    }

    __attribute__((visibility("default")))
    void ufeel_debug_show_directions(void *frame, UFeelBoolPair *arr, int size)
    {
        cv::Mat &cvFrame = *static_cast<cv::Mat*>(frame);
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
        int lineHeight = 25;

        for (int i = 0; i < size; i++)
        {
            std::ostringstream oss;
            oss << arr[i].key << ": " << arr[i].value;

            cv::putText(cvFrame, oss.str(), cv::Point(20, y),
                        cv::FONT_HERSHEY_SIMPLEX, 0.6,
                        cv::Scalar(255, 255, 255), 2);

            y += lineHeight;
        }

        cv::imshow("Eye Tracking Detection", cvFrame);
    }

    __attribute__((visibility("default")))
    void ufeel_debug_show_speech(void *frame, char *speech)
    {
        if (!frame)
            return;

        cv::Mat &cvFrame = *static_cast<cv::Mat*>(frame);

        std::ostringstream oss;
        oss << "Current speech: " << speech;

        cv::putText(cvFrame, oss.str(), cv::Point(20, 30),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6,
                    cv::Scalar(255, 255, 255), 2);

        cv::imshow("Debug UFeel", cvFrame);
    }

    __attribute__((visibility("default")))
    void ufeel_debug_show_frame(void *frame)
    {
        if (!frame)
            return;

        cv::Mat &cvFrame = *static_cast<cv::Mat*>(frame);
        cv::imshow("Debug UFeel", cvFrame);
    }

    __attribute__((visibility("default")))
    int ufeel_debug_wait_key(int delay)
    {
        return cv::waitKey(delay);
    }
}