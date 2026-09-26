// NOLINTBEGIN(misc-include-cleaner)
#include <string>
#include <torch/torch.h>

template<typename Model> auto load_checkpoint(Model model, const std::string& path, torch::Device device = torch::kCPU) -> Model
{
    torch::serialize::InputArchive archive;
    archive.load_from(path);

    model->load(archive);

    model->to(device);
    model->eval();

    return model;
}

// NOLINTEND(misc-include-cleaner)