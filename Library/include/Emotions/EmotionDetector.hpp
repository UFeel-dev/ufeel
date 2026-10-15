#ifndef EMOTIONS_EMOTIONDETECTOR_HPP
#define EMOTIONS_EMOTIONDETECTOR_HPP

#include "CheckPointLoader.hpp"
#include "EmotionModel.hpp"
#include "Inference/InferencePipeline.hpp"

#include <filesystem>
#include <fstream>
#include <memory>
#include <opencv2/opencv.hpp>
#include <stdexcept>
#include <string>
#include <torch/script.h>
#include <unordered_map>
#include <vector>
#include <yaml-cpp/yaml.h>

class EmotionDetector
{
    public:

        EmotionDetector(
            const std::string& modelPath, const std::string& configPath, std::string device = "cpu"
        );
        ~EmotionDetector();

        EmotionDetector(const EmotionDetector&) = delete;
        auto operator=(const EmotionDetector&) -> EmotionDetector& = delete;

        EmotionDetector(EmotionDetector&&) = delete;
        auto operator=(EmotionDetector&&) -> EmotionDetector& = delete;

        void toggleEmotionDetection(bool state);
        auto process(const cv::Mat& image) -> std::map<std::string, float>;
        void close();

    private:

        static auto loadConfig(const std::string& configPath) -> YAML::Node;

        bool processEnable_ = false;

        torch::Device device_;
        EmotionModel model_ = nullptr;

        std::shared_ptr<InferencePipeline> pipeline_;

        const std::vector<std::string> classLabels_{
            "angry",
            "contemptuous",
            "disgusted",
            "fearful",
            "happy",
            "neutral",
            "sad",
            "surprised"};
};

#endif // EMOTIONS_EMOTIONDETECTOR_HPP
