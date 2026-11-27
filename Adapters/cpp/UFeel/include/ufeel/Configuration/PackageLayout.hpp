#ifndef UFEEL_CONFIGURATION_PACKAGELAYOUT_HPP
#define UFEEL_CONFIGURATION_PACKAGELAYOUT_HPP

#include <cstdint>
#include <filesystem>

namespace ufeel
{

enum class SpeechLanguage : std::uint8_t
{
    French,
    English,
    German,
};

struct PackageLayout
{
    std::filesystem::path rootDirectory;
    std::filesystem::path modelDirectory;
    std::filesystem::path speechModelPath;

    [[nodiscard]] static auto fromRoot(
        const std::filesystem::path& rootDirectory, SpeechLanguage language = SpeechLanguage::French
    ) -> PackageLayout;
};

} // namespace ufeel

#endif // UFEEL_CONFIGURATION_PACKAGELAYOUT_HPP
