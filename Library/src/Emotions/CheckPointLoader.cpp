#include "Emotions/CheckPointLoader.hpp"

#include "Emotions/EmotionModel.hpp"

#include <c10/core/Device.h>
#include <string>
#include <torch/headeronly/core/DeviceType.h>
#include <torch/serialize/input-archive.h>

auto loadCheckpoint(
    EmotionModel model, const std::string& path, torch::Device device
) -> EmotionModel
{
    torch::serialize::InputArchive archive;
    archive.load_from(path);

    model->load(archive);

    model->to(device);
    model->eval();

    return model;
}
