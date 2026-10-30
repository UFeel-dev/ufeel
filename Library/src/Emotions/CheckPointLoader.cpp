#include "Emotions/CheckPointLoader.hpp"

#include "Emotions/EmotionModel.hpp"
#include "torch/optim/optimizer.h"
#include "torch/utils.h"

#include <bit>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <istream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{

static_assert(std::endian::native == std::endian::little, "Weights are stored little-endian");

template <typename T>
auto read(
    std::istream& in
) -> T
{
    T value{};
    in.read(reinterpret_cast<char*>(&value), sizeof(T));
    if (!in)
    {
        throw std::runtime_error("Truncated weights file");
    }
    return value;
}

} // namespace

void loadCheckpoint(
    EmotionModel& model, const std::filesystem::path& path
)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        throw std::runtime_error("Unable to open weights file: " + path.string());
    }

    std::unordered_map<std::string, torch::Tensor> pending;

    for (const auto& item : model->named_parameters())
    {
        pending.emplace(item.key(), item.value());
    }
    for (const auto& item : model->named_buffers())
    {
        if (!item.key().ends_with("num_batches_tracked"))
        {
            pending.emplace(item.key(), item.value());
        }
    }

    const torch::NoGradGuard noGrad;

    const auto count = read<std::uint32_t>(file);
    for (std::uint32_t i = 0; i < count; ++i)
    {
        std::string name(read<std::uint32_t>(file), '\0');
        file.read(name.data(), static_cast<std::streamsize>(name.size()));

        std::vector<std::int64_t> shape(read<std::uint32_t>(file));
        for (auto& dim : shape)
        {
            dim = read<std::int64_t>(file);
        }
        const auto numel = read<std::uint64_t>(file);

        const auto it = pending.find(name);
        if (it == pending.end())
        {
            throw std::runtime_error("Unexpected tensor in weights file: " + name);
        }

        auto& target = it->second;
        if (target.sizes().vec() != shape || static_cast<std::uint64_t>(target.numel()) != numel)
        {
            throw std::runtime_error("Shape mismatch for tensor: " + name);
        }

        file.read(
            reinterpret_cast<char*>(target.data_ptr<float>()),
            static_cast<std::streamsize>(numel * sizeof(float))
        );
        if (!file)
        {
            throw std::runtime_error("Truncated weights file at tensor: " + name);
        }

        pending.erase(it);
    }

    if (!pending.empty())
    {
        throw std::runtime_error("Tensor missing from weights file: " + pending.begin()->first);
    }
}
