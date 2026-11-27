#include "ufeel/Configuration/PackageLayout.hpp"

#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace ufeel
{

namespace
{

auto languageCode(
    SpeechLanguage language
) -> std::u8string_view
{
    switch (language)
    {
    case SpeechLanguage::French:
        return u8"fr";
    case SpeechLanguage::English:
        return u8"en";
    case SpeechLanguage::German:
        return u8"de";
    }

    throw std::invalid_argument("Unknown speech language.");
}

auto findSpeechModel(
    const std::filesystem::path& modelDirectory, SpeechLanguage language
) -> std::optional<std::filesystem::path>
{
    constexpr std::u8string_view prefix = u8"vosk-model-";

    const auto speechDirectory = modelDirectory / "SpeechToText";
    if (!std::filesystem::is_directory(speechDirectory))
    {
        return std::nullopt;
    }

    const std::u8string marker = std::u8string(u8"-").append(languageCode(language)).append(u8"-");
    std::optional<std::filesystem::path> latest;
    for (const auto& entry : std::filesystem::directory_iterator(speechDirectory))
    {
        const std::u8string name = entry.path().filename().u8string();
        if (!entry.is_directory() || !name.starts_with(prefix) ||
            name.find(marker, prefix.size()) == std::u8string::npos)
        {
            continue;
        }

        if (!latest || name > latest->filename().u8string())
        {
            latest = entry.path();
        }
    }

    return latest;
}

} // namespace

auto PackageLayout::fromRoot(
    const std::filesystem::path& rootDirectory, SpeechLanguage language
) -> PackageLayout
{
    if (rootDirectory.empty())
    {
        throw std::invalid_argument("A package directory is required.");
    }

    auto root = std::filesystem::absolute(rootDirectory).lexically_normal();
    if (!std::filesystem::is_directory(root))
    {
        throw std::filesystem::filesystem_error(
            "UFeel package directory not found",
            root,
            std::make_error_code(std::errc::no_such_file_or_directory)
        );
    }

    auto models = root / "models";
    if (!std::filesystem::is_directory(models))
    {
        models = root;
    }

    auto speechModel = findSpeechModel(models, language).value_or(models);

    return {
        .rootDirectory = std::move(root),
        .modelDirectory = std::move(models),
        .speechModelPath = std::move(speechModel),
    };
}

} // namespace ufeel
