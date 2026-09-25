#include <torch/torch.h>

template<typename Model> Model load_checkpoint(Model model, const std::string& path, torch::Device device = torch::kCPU)
{
    torch::serialize::InputArchive archive;
    archive.load_from(path);

    model->load(archive);

    model->to(device);
    model->eval();

    return model;
}