#include "Emotions/EmotionDetector.hpp"

#include "Emotions/CheckPointLoader.hpp"
#include "opencv2/core.hpp"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/types.hpp"
#include "opencv2/dnn/dnn.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/objdetect/face.hpp"

#include <ATen/core/TensorBody.h>
#include <ATen/core/ivalue.h>
#include <ATen/core/ivalue_inl.h>
#include <algorithm>
#include <c10/util/ArrayRef.h>
#include <c10/util/Exception.h>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <map>
#include <string>
#include <torch/csrc/autograd/generated/variable_factories.h>
#include <torch/csrc/jit/serialization/import.h>
#include <torch/csrc/jit/serialization/pickler.h>
#include <torch/types.h>
#include <vector>


EmotionDetector::EmotionDetector(
    const std::string& modelPath, const std::string& configPath, std::string device
)
    : device_(torch::Device(device))
{
    YAML::Node config = loadConfig(configPath);

    auto model_cfg = config["model"];
    auto data_cfg = config["data"];

    model_ = EmotionModel(model_cfg["backbone_name"].as<std::string>());

    model_ =
        loadCheckpoint(model_, "models/Emotions/model_weights.bin", device_);

    auto detector = std::make_shared<YuNetFaceDetector>(
        "models/EyeTracking/face_detection_yunet_2023mar.onnx", 0.9f, 0.3f, 5000
    );

    auto face_crop = std::make_shared<MTCNNCropTransform>(data_cfg["img_size"].as<int>(), detector);

    pipeline_ = std::make_shared<InferencePipeline>(
        model_, device_, face_crop, data_cfg["img_size"].as<int>()
    );

    std::unordered_map<std::string, float> thresholds;

    const auto thresholds_node = model_cfg["thresholds"];

    for (const auto& node : thresholds_node)
    {
        const auto emotion = node.first.as<std::string>();
        const auto threshold = node.second.as<float>();

        thresholds[emotion] = threshold;
    }
    pipeline_->set_calibration(HeadCalibration(thresholds));

    processEnable_ = false;
}

EmotionDetector::~EmotionDetector()
{
    close();
}

auto EmotionDetector::loadConfig(
    const std::string& configPath
) -> YAML::Node
{
    namespace fs = std::filesystem;

    fs::path path(configPath);

    if (!fs::exists(path))
    {
        std::cerr << "Configuration file not found: " << path << '\n';

        throw std::runtime_error("Configuration file not found: " + path.string());
    }

    try
    {
        return YAML::LoadFile(path.string());
    }
    catch (const YAML::Exception& e)
    {
        throw std::runtime_error("Failed to parse configuration file: " + std::string(e.what()));
    }
}

void EmotionDetector::toggleEmotionDetection(
    bool state
)
{
    processEnable_ = state;
    std::cout << "[EmotionDetector] Emotion detection " << (state ? "enabled" : "disabled") << '\n';
}

auto EmotionDetector::process(
    const cv::Mat& image
) -> std::map<std::string, float>
{
    if (image.empty())
    {
        return {};
    }

    auto out = pipeline_->predict(image);

    if (out.binary.empty())
    {
        return {};
    }

    std::map<std::string, float> selected;

    for (const auto& [emotion, value] : out.heads)
    {
        auto it = out.binary.find(emotion);

        if (it != out.binary.end())
        {
            const bool enabled = it->second;

            if (enabled)
            {
                selected.emplace(emotion, value);
            }
        }
    }
    return selected;
}

void EmotionDetector::close() {}
