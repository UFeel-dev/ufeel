#ifndef UFEEL_CONFIGURATION_CALIBRATIONPROFILESTORE_HPP
#define UFEEL_CONFIGURATION_CALIBRATIONPROFILESTORE_HPP

#include "ufeel/Native/Processor.hpp"

#include <cstdint>
#include <filesystem>
#include <string_view>

namespace ufeel
{

enum class ProfileLoadResult : std::uint8_t
{
    Missing,
    Loaded,
    Invalid,
};

class CalibrationProfileStore
{
public:

    static constexpr std::string_view fileName = "eyeTracking-profile-v4.txt";

    explicit CalibrationProfileStore(const std::filesystem::path& path);

    [[nodiscard]] static auto createDefault() -> CalibrationProfileStore;

    [[nodiscard]] auto path() const -> const std::filesystem::path&;

    [[nodiscard]] auto load(Processor& processor) const -> ProfileLoadResult;
    [[nodiscard]] auto save(Processor& processor) const -> bool;

private:

    std::filesystem::path path_;
};

} // namespace ufeel

#endif // UFEEL_CONFIGURATION_CALIBRATIONPROFILESTORE_HPP
