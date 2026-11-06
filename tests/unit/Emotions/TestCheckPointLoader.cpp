#include "Emotions/CheckPointLoader.hpp"
#include "Emotions/EmotionModel.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <ios>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace
{

class TemporaryCheckpoint
{
public:

    TemporaryCheckpoint()
    {
        const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() /
                ("ufeel-checkpoint-" + std::to_string(timestamp) + ".bin");
    }

    ~TemporaryCheckpoint()
    {
        std::error_code error;
        std::filesystem::remove(path_, error);
    }

    [[nodiscard]] auto path() const -> const std::filesystem::path&
    {
        return path_;
    }

    auto write(
        const std::vector<char>& bytes
    ) const -> void
    {
        std::ofstream file(path_, std::ios::binary);
        file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    }

private:

    std::filesystem::path path_;
};

template <typename T>
auto append(
    std::vector<char>& bytes, T value
) -> void
{
    const auto* data = reinterpret_cast<const char*>(&value);
    bytes.insert(bytes.end(), data, data + sizeof(value));
}

auto entryHeader(
    const std::string& name, const std::vector<std::int64_t>& shape, std::uint64_t elementCount
) -> std::vector<char>
{
    std::vector<char> bytes;
    append(bytes, std::uint32_t{1});
    append(bytes, static_cast<std::uint32_t>(name.size()));
    bytes.insert(bytes.end(), name.begin(), name.end());
    append(bytes, static_cast<std::uint32_t>(shape.size()));
    for (const auto dimension : shape)
    {
        append(bytes, dimension);
    }
    append(bytes, elementCount);
    return bytes;
}

} // namespace

TEST(
    CheckPointLoaderTest, ThrowsWhenCheckpointDoesNotExist
)
{
    EmotionModel model("resnet34");
    const auto path =
        std::filesystem::temp_directory_path() / "ufeel-checkpoint-that-does-not-exist.bin";

    EXPECT_THROW(loadCheckpoint(model, path), std::runtime_error);
}

TEST(
    CheckPointLoaderTest, ThrowsWhenCheckpointHeaderIsTruncated
)
{
    EmotionModel model("resnet34");
    TemporaryCheckpoint checkpoint;
    checkpoint.write({0x01, 0x00});

    EXPECT_THROW(loadCheckpoint(model, checkpoint.path()), std::runtime_error);
}

TEST(
    CheckPointLoaderTest, ThrowsWhenCheckpointContainsUnexpectedTensor
)
{
    EmotionModel model("resnet34");
    TemporaryCheckpoint checkpoint;
    checkpoint.write(entryHeader("unexpected.tensor", {}, 0));

    EXPECT_THROW(loadCheckpoint(model, checkpoint.path()), std::runtime_error);
}

TEST(
    CheckPointLoaderTest, ThrowsWhenTensorShapeDoesNotMatch
)
{
    EmotionModel model("resnet34");
    TemporaryCheckpoint checkpoint;
    checkpoint.write(entryHeader("heads.angry.linear.bias", {2}, 2));

    EXPECT_THROW(loadCheckpoint(model, checkpoint.path()), std::runtime_error);
}

TEST(
    CheckPointLoaderTest, ThrowsWhenTensorDataIsTruncated
)
{
    EmotionModel model("resnet34");
    TemporaryCheckpoint checkpoint;
    checkpoint.write(entryHeader("heads.angry.linear.bias", {1}, 1));

    EXPECT_THROW(loadCheckpoint(model, checkpoint.path()), std::runtime_error);
}

TEST(
    CheckPointLoaderTest, ThrowsWhenCheckpointOmitsModelTensors
)
{
    EmotionModel model("resnet34");
    TemporaryCheckpoint checkpoint;
    checkpoint.write({0x00, 0x00, 0x00, 0x00});

    EXPECT_THROW(loadCheckpoint(model, checkpoint.path()), std::runtime_error);
}
