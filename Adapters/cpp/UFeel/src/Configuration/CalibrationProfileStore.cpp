#include "ufeel/Configuration/CalibrationProfileStore.hpp"

#include "ufeel/Native/Processor.hpp"

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <system_error>

namespace ufeel
{

namespace
{

auto environmentPath(
    const char* name
) -> std::optional<std::filesystem::path>
{
    // NOLINTNEXTLINE(concurrency-mt-unsafe): UFeel never writes the environment.
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0')
    {
        return std::nullopt;
    }
    return std::filesystem::path(value);
}

auto userDataDirectory() -> std::filesystem::path
{
#ifdef _WIN32
    if (auto localAppData = environmentPath("LOCALAPPDATA"))
    {
        return *localAppData;
    }
#else
#ifdef __APPLE__
    if (auto home = environmentPath("HOME"))
    {
        return *home / "Library" / "Application Support";
    }
#else
    if (auto dataHome = environmentPath("XDG_DATA_HOME"); dataHome && dataHome->is_absolute())
    {
        return *dataHome;
    }
    if (auto home = environmentPath("HOME"))
    {
        return *home / ".local" / "share";
    }
#endif
#endif

    throw std::runtime_error("Cannot determine the per-user application data directory.");
}

auto absoluteProfilePath(
    const std::filesystem::path& path
) -> std::filesystem::path
{
    if (path.empty())
    {
        throw std::invalid_argument("A profile path is required.");
    }
    return std::filesystem::absolute(path).lexically_normal();
}

} // namespace

CalibrationProfileStore::CalibrationProfileStore(
    const std::filesystem::path& path
)
    : path_(absoluteProfilePath(path))
{
}

auto CalibrationProfileStore::createDefault() -> CalibrationProfileStore
{
    return CalibrationProfileStore(userDataDirectory() / "UFeel" / fileName);
}

auto CalibrationProfileStore::path() const -> const std::filesystem::path&
{
    return path_;
}

auto CalibrationProfileStore::load(
    Processor& processor
) const -> ProfileLoadResult
{
    std::error_code error;
    if (!std::filesystem::is_regular_file(path_, error))
    {
        return ProfileLoadResult::Missing;
    }

    return processor.loadEyeTrackingCalibrationProfile(path_) ? ProfileLoadResult::Loaded
                                                              : ProfileLoadResult::Invalid;
}

auto CalibrationProfileStore::save(
    Processor& processor
) const -> bool
{
    std::error_code error;
    std::filesystem::create_directories(path_.parent_path(), error);
    return !error && processor.saveEyeTrackingCalibrationProfile(path_);
}

} // namespace ufeel
