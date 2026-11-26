#include "Adapters/cpp/TestSupport/TemporaryDirectory.hpp"
#include "ufeel/Configuration/PackageLayout.hpp"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <stdexcept>

namespace
{

void createSpeechModels(
    const std::filesystem::path& root
)
{
    const auto speech = root / "models" / "SpeechToText";
    std::filesystem::create_directories(speech / "vosk-model-small-fr-0.10");
    std::filesystem::create_directories(speech / "vosk-model-small-fr-0.22");
    std::filesystem::create_directories(speech / "vosk-model-small-en-us-0.15");
    std::filesystem::create_directories(speech / "vosk-model-fr-9.99");
    std::ofstream(speech / "vosk-model-small-fr-9.99") << "not a directory";
}

} // namespace

TEST(
    PackageLayoutTest, SelectsTheLatestSpeechModelOfTheLanguage
)
{
    const TemporaryDirectory package;
    createSpeechModels(package.path());
    const auto models = package.path() / "models";

    const auto french = ufeel::PackageLayout::fromRoot(package.path());
    const auto english =
        ufeel::PackageLayout::fromRoot(package.path(), ufeel::SpeechLanguage::English);

    EXPECT_EQ(french.rootDirectory, package.path());
    EXPECT_EQ(french.modelDirectory, models);
    EXPECT_EQ(french.speechModelPath, models / "SpeechToText" / "vosk-model-small-fr-0.22");
    EXPECT_EQ(english.speechModelPath, models / "SpeechToText" / "vosk-model-small-en-us-0.15");
}

TEST(
    PackageLayoutTest, FallsBackToTheModelDirectoryWithoutASpeechModel
)
{
    const TemporaryDirectory package;
    createSpeechModels(package.path());

    const auto german =
        ufeel::PackageLayout::fromRoot(package.path(), ufeel::SpeechLanguage::German);

    EXPECT_EQ(german.speechModelPath, package.path() / "models");
}

TEST(
    PackageLayoutTest, UsesTheRootWithoutAModelDirectory
)
{
    const TemporaryDirectory package;

    const auto layout = ufeel::PackageLayout::fromRoot(package.path());

    EXPECT_EQ(layout.modelDirectory, package.path());
    EXPECT_EQ(layout.speechModelPath, package.path());
}

TEST(
    PackageLayoutTest, RejectsMissingPackages
)
{
    const TemporaryDirectory package;

    EXPECT_THROW(
        static_cast<void>(ufeel::PackageLayout::fromRoot(package.path() / "missing")),
        std::filesystem::filesystem_error
    );
    EXPECT_THROW(static_cast<void>(ufeel::PackageLayout::fromRoot("")), std::invalid_argument);
}
