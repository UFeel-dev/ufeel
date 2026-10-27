#include "Emotions/CheckPointLoader.hpp"

#include "Emotions/EmotionModel.hpp"

#include <c10/core/Device.h>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <torch/headeronly/core/DeviceType.h>
#include <torch/serialize/input-archive.h>
#include <torch/torch.h>
#include <unordered_map>
#include <vector>

auto loadCheckpoint(
    EmotionModel model, const std::string& path, torch::Device device
) -> EmotionModel
{
    if (!model)
    {
        std::cout << "EmotionModel is null." << std::endl;
    }

    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        std::cout << "Unable to open checkpoint: " <<  path << std::endl;
    }

    auto readExact = [&file](char* data, std::streamsize size) {
        file.read(data, size);
        if (!file)
        {
            std::cout << "Invalid or truncated weights file." << std::endl;
        }
    };

    auto readU32 = [&]() -> std::uint32_t {
        unsigned char b[4];
        readExact(reinterpret_cast<char*>(b), 4);

        return static_cast<std::uint32_t>(b[0]) | (static_cast<std::uint32_t>(b[1]) << 8) |
               (static_cast<std::uint32_t>(b[2]) << 16) | (static_cast<std::uint32_t>(b[3]) << 24);
    };

    auto readU64 = [&]() -> std::uint64_t {
        unsigned char b[8];
        readExact(reinterpret_cast<char*>(b), 8);

        std::uint64_t value = 0;
        for (int i = 0; i < 8; ++i)
        {
            value |= static_cast<std::uint64_t>(b[i]) << (8 * i);
        }
        return value;
    };

    auto readI64 = [&]() -> std::int64_t { return static_cast<std::int64_t>(readU64()); };

    // Paramètres et buffers enregistrés dans le module C++.
    std::unordered_map<std::string, torch::Tensor> targets;

    for (const auto& item : model->named_parameters(true))
    {
        targets.emplace(item.key(), item.value());
    }

    for (const auto& item : model->named_buffers(true))
    {
        targets.emplace(item.key(), item.value());
    }

    const auto count = readU32();

    torch::NoGradGuard noGrad;

    for (std::uint32_t i = 0; i < count; ++i)
    {
        const auto nameLength = readU32();

        if (nameLength == 0 || nameLength > 4096)
        {
            std::cout << "Invalid tensor name." << std::endl;
        }

        std::string name(nameLength, '\0');
        readExact(name.data(), static_cast<std::streamsize>(nameLength));

        const auto rank = readU32();
        if (rank > 16)
        {
            std::cout << "Invalid tensor dimensions." << std::endl;
        }

        std::vector<std::int64_t> shape;
        shape.reserve(rank);

        for (std::uint32_t d = 0; d < rank; ++d)
        {
            shape.push_back(readI64());
        }

        const auto numel = readU64();

        std::uint64_t expected = 1;
        for (const auto dim : shape)
        {
            if (dim < 0 || (dim != 0 && expected > UINT64_MAX / static_cast<std::uint64_t>(dim)))
            {
                std::cout << "Invalid dimensions: " <<  name << std::endl;
            }
            expected *= static_cast<std::uint64_t>(dim);
        }

        if (expected != numel ||
            numel >
                static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max() / sizeof(float)))
        {
            std::cout << "Invalid tensor size: " <<  name << std::endl;
        }

        auto it = targets.find(name);

        if (it == targets.end())
        {
            // Équivalent à l'ignorance d'une clé inattendue avec strict=False.
            file.seekg(static_cast<std::streamoff>(numel * sizeof(float)), std::ios::cur);
            if (!file)
            {
                std::cout << "rruncated file: " <<  name << std::endl;
            }
            continue;
        }

        auto target = it->second;

        if (target.sizes().vec() != shape)
        {
            std::cout << "Incompatible dimensions for " <<  name << std::endl;
        }

        std::vector<float> values(static_cast<std::size_t>(numel));
        readExact(
            reinterpret_cast<char*>(values.data()),
            static_cast<std::streamsize>(numel * sizeof(float))
        );

        auto source =
            torch::from_blob(values.data(), shape, torch::TensorOptions().dtype(torch::kFloat32));

        target.copy_(source.to(target.device(), target.scalar_type()));
    }

    model->to(device);
    model->eval();

    return model;
}
