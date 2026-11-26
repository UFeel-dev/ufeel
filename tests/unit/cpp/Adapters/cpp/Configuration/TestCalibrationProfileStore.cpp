#include "Adapters/cpp/TestSupport/FakeProcessor.hpp"
#include "Adapters/cpp/TestSupport/TemporaryDirectory.hpp"
#include "ufeel/Configuration/CalibrationProfileStore.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <optional>
#include <stdexcept>
#include <stdlib.h> // NOLINT(modernize-deprecated-headers): setenv and unsetenv are POSIX
#include <string>

namespace
{

class ScopedEnvironment
{
public:

    ScopedEnvironment(
        const char* name, const std::string& value
    )
        : name_(name)
        , previous_(read(name))
    {
        setenv(name, value.c_str(), 1); // NOLINT(concurrency-mt-unsafe)
    }

    ScopedEnvironment(const ScopedEnvironment&) = delete;
    ScopedEnvironment(ScopedEnvironment&&) = delete;
    auto operator=(const ScopedEnvironment&) -> ScopedEnvironment& = delete;
    auto operator=(ScopedEnvironment&&) -> ScopedEnvironment& = delete;

    ~ScopedEnvironment()
    {
        if (previous_)
        {
            setenv(name_, previous_->c_str(), 1); // NOLINT(concurrency-mt-unsafe)
        }
        else
        {
            unsetenv(name_); // NOLINT(concurrency-mt-unsafe)
        }
    }

private:

    static auto read(
        const char* name
    ) -> std::optional<std::string>
    {
        const char* value = std::getenv(name); // NOLINT(concurrency-mt-unsafe)
        return value == nullptr ? std::nullopt : std::optional<std::string>(value);
    }

    const char* name_;
    std::optional<std::string> previous_;
};

} // namespace

TEST(
    CalibrationProfileStoreTest, ReportsMissingProfiles
)
{
    const TemporaryDirectory directory;
    auto [backend, processor] = makeFakeProcessor();
    const ufeel::CalibrationProfileStore profiles(directory.path() / "profile.txt");

    EXPECT_EQ(profiles.load(processor), ufeel::ProfileLoadResult::Missing);
}

TEST(
    CalibrationProfileStoreTest, LoadsExistingProfiles
)
{
    const TemporaryDirectory directory;
    auto [backend, processor] = makeFakeProcessor();
    const ufeel::CalibrationProfileStore profiles(directory.path() / "profile.txt");
    std::ofstream(profiles.path()) << "profile";

    EXPECT_EQ(profiles.load(processor), ufeel::ProfileLoadResult::Loaded);

    backend->profileLoaded = false;
    EXPECT_EQ(profiles.load(processor), ufeel::ProfileLoadResult::Invalid);
}

TEST(
    CalibrationProfileStoreTest, CreatesTheDirectoryWhenSaving
)
{
    const TemporaryDirectory directory;
    auto [backend, processor] = makeFakeProcessor();
    const ufeel::CalibrationProfileStore profiles(directory.path() / "a" / "b" / "profile.txt");

    EXPECT_TRUE(profiles.save(processor));
    EXPECT_TRUE(std::filesystem::is_directory(directory.path() / "a" / "b"));

    backend->profileSaved = false;
    EXPECT_FALSE(profiles.save(processor));
}

TEST(
    CalibrationProfileStoreTest, FailsToSaveWhenTheDirectoryCannotBeCreated
)
{
    const TemporaryDirectory directory;
    auto [backend, processor] = makeFakeProcessor();
    std::ofstream(directory.path() / "file") << "not a directory";
    const ufeel::CalibrationProfileStore profiles(directory.path() / "file" / "profile.txt");

    EXPECT_FALSE(profiles.save(processor));
}

TEST(
    CalibrationProfileStoreTest, UsesAbsolutePaths
)
{
    const ufeel::CalibrationProfileStore profiles("relative/../profile.txt");

    EXPECT_TRUE(profiles.path().is_absolute());
    EXPECT_EQ(profiles.path().filename(), "profile.txt");
    EXPECT_EQ(profiles.path().parent_path(), std::filesystem::current_path());
    EXPECT_THROW(ufeel::CalibrationProfileStore(""), std::invalid_argument);
}

TEST(
    CalibrationProfileStoreTest, StoresTheDefaultProfileInTheUserDataDirectory
)
{
    const TemporaryDirectory directory;
    const ScopedEnvironment dataHome("XDG_DATA_HOME", directory.path().string());

    EXPECT_EQ(
        ufeel::CalibrationProfileStore::createDefault().path(),
        directory.path() / "UFeel" / "eyeTracking-profile-v4.txt"
    );
}

TEST(
    CalibrationProfileStoreTest, FallsBackToTheHomeDirectory
)
{
    const TemporaryDirectory directory;
    const ScopedEnvironment dataHome("XDG_DATA_HOME", "relative");
    const ScopedEnvironment home("HOME", directory.path().string());

    EXPECT_EQ(
        ufeel::CalibrationProfileStore::createDefault().path(),
        directory.path() / ".local" / "share" / "UFeel" / "eyeTracking-profile-v4.txt"
    );
}
