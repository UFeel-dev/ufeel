#ifndef ADAPTERS_CPP_TESTSUPPORT_TEMPORARYDIRECTORY_HPP
#define ADAPTERS_CPP_TESTSUPPORT_TEMPORARYDIRECTORY_HPP

#include <chrono>
#include <filesystem>
#include <gtest/gtest.h>
#include <string>
#include <system_error>

class TemporaryDirectory
{
public:

    TemporaryDirectory()
        : path_(
              std::filesystem::temp_directory_path() /
              (std::string("ufeel-cpp-adapter-") +
               ::testing::UnitTest::GetInstance()->current_test_info()->name() + "-" +
               std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))
          )
    {
        std::filesystem::create_directories(path_);
    }

    TemporaryDirectory(const TemporaryDirectory&) = delete;
    TemporaryDirectory(TemporaryDirectory&&) = delete;
    auto operator=(const TemporaryDirectory&) -> TemporaryDirectory& = delete;
    auto operator=(TemporaryDirectory&&) -> TemporaryDirectory& = delete;

    ~TemporaryDirectory()
    {
        std::error_code error;
        std::filesystem::remove_all(path_, error);
    }

    [[nodiscard]] auto path() const -> const std::filesystem::path&
    {
        return path_;
    }

private:

    std::filesystem::path path_;
};

#endif // ADAPTERS_CPP_TESTSUPPORT_TEMPORARYDIRECTORY_HPP
