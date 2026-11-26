cmake_minimum_required(VERSION 3.24)

project(ufeel_test_harness LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(UFEEL_PACKAGE_DIR "" CACHE PATH "UFeel package to run against, built by ./build.sh all")
if(NOT EXISTS "${UFEEL_PACKAGE_DIR}/include/ufeel.h")
    message(FATAL_ERROR
        "Set UFEEL_PACKAGE_DIR to a UFeel package, or run ./build.sh test-harness --adapter cpp")
endif()

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../Adapters/cpp/UFeel" ufeel)

include(FetchContent)

FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
    URL_HASH SHA256=aea98ecf5bc5c5e0b789a76de0083a21a70457050ea4cc2aec7566935f5e258e
    FIND_PACKAGE_ARGS 5.5
)
FetchContent_MakeAvailable(raylib)

add_executable(ufeel_test_harness
    src/CameraFeed.cpp
    src/Harness.cpp
    src/HarnessCalibration.cpp
    src/HarnessSpeech.cpp
    src/HarnessResults.cpp
    src/Ui.cpp
    src/main.cpp
)

target_include_directories(ufeel_test_harness PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_definitions(ufeel_test_harness PRIVATE UFEEL_PACKAGE_DIR="${UFEEL_PACKAGE_DIR}")
target_link_libraries(ufeel_test_harness PRIVATE ufeel::ufeel raylib)
