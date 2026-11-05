#include "Emotions/EmotionDetector.hpp"

#include "Emotions/CheckPointLoader.hpp"
#include "Emotions/EmotionModel.hpp"
#include "Emotions/Inference/FaceCrop.hpp"
#include "Emotions/Inference/FaceDetector.hpp"
#include "Emotions/Inference/HeadCalibration.hpp"
#include "Emotions/Inference/InferencePipeline.hpp"
#include "opencv2/core/hal/interface.h"
#include "opencv2/core/mat.hpp"
#include "torch/headeronly/core/DeviceType.h"

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/yaml.h> // ! needs to be included for templates to compile

namespace
{

constexpr auto kDevice = torch::kCPU;

struct EmotionConfig
{
    std::string backboneName;
    int imageSize;
    std::map<std::string, float> thresholds;
};

auto loadConfig(
    const std::filesystem::path& path
) -> EmotionConfig
{
    const auto root = YAML::LoadFile(path.string());
    const auto model = root["model"];

    return {
        .backboneName = model["backbone_name"].as<std::string>(),
        .imageSize = root["data"]["img_size"].as<int>(),
        .thresholds = model["thresholds"].as<std::map<std::string, float>>(),
    };
}

} // namespace

EmotionDetector::EmotionDetector(
    const std::filesystem::path& modelDirectory
)
{
    const auto directory = modelDirectory / "Emotions";
    const auto config = loadConfig(directory / "binary_affectnet_resnet34_pretrained.yaml");

    EmotionModel model(config.backboneName);
    loadCheckpoint(model, directory / "model_weights.bin");

    FaceCrop faceCrop(
        FaceDetector((directory / "face_detection_yunet_2023mar.onnx").string()), config.imageSize
    );

    pipeline_ = std::make_unique<InferencePipeline>(std::move(model), kDevice, std::move(faceCrop));
    calibration_ = HeadCalibration(config.thresholds);
}

EmotionDetector::~EmotionDetector() = default;

auto EmotionDetector::process(
    const cv::Mat& image
) -> std::map<std::string, float>
{
    if (image.empty() || image.type() != CV_8UC3)
    {
        return {};
    }

    return calibration_.select(pipeline_->predict(image));
}
