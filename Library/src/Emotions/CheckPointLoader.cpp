#include <c10/core/Device.h>
#include <string>
#include <torch/headeronly/core/DeviceType.h>
#include <torch/serialize/input-archive.h>

template <typename Model>
static auto load_checkpoint(
    Model model, const std::string& path, torch::Device device = torch::kCPU
) -> Model
{
    torch::serialize::InputArchive archive;
    archive.load_from(path);

    model->load(archive);

    model->to(device);
    model->eval();

    return model;
}
