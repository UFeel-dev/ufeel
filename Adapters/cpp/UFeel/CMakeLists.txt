cmake_minimum_required(VERSION 3.20)

if(NOT TARGET ufeel_wrapper)
    set(UFEEL_PACKAGE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../.." CACHE PATH
        "UFeel package root, holding the ufeel_wrapper library and include/ufeel.h")

    cmake_path(ABSOLUTE_PATH UFEEL_PACKAGE_DIR NORMALIZE OUTPUT_VARIABLE ufeel_package_dir)
    set(ufeel_wrapper_name
        "${CMAKE_SHARED_LIBRARY_PREFIX}ufeel_wrapper${CMAKE_SHARED_LIBRARY_SUFFIX}")

    if(NOT EXISTS "${ufeel_package_dir}/${ufeel_wrapper_name}" OR
       NOT EXISTS "${ufeel_package_dir}/include/ufeel.h")
        message(FATAL_ERROR
            "No UFeel package in '${ufeel_package_dir}': set UFEEL_PACKAGE_DIR to the directory "
            "holding ${ufeel_wrapper_name} and include/ufeel.h")
    endif()

    add_library(ufeel_wrapper SHARED IMPORTED GLOBAL)
    set_target_properties(ufeel_wrapper PROPERTIES
        IMPORTED_LOCATION "${ufeel_package_dir}/${ufeel_wrapper_name}"
        INTERFACE_INCLUDE_DIRECTORIES "${ufeel_package_dir}/include"
    )
endif()

find_package(Threads REQUIRED)

add_library(ufeel_cpp STATIC
    src/Calibration/CalibrationResult.cpp
    src/Calibration/EyeTrackingCalibration.cpp
    src/Configuration/CalibrationProfileStore.cpp
    src/Configuration/PackageLayout.cpp
    src/Model/Detection.cpp
    src/Model/EmotionResult.cpp
    src/Model/Status.cpp
    src/Native/Error.cpp
    src/Native/Processor.cpp
    src/Pipeline/EyeTrackingSmoother.cpp
    src/Pipeline/FeatureDemand.cpp
    src/Pipeline/FeatureScheduler.cpp
    src/Pipeline/LatestFrameCapture.cpp
    src/Pipeline/Session.cpp
    src/Rules/RuleSet.cpp
    src/Text/Text.cpp
)
add_library(ufeel::ufeel ALIAS ufeel_cpp)

target_include_directories(ufeel_cpp PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(ufeel_cpp PUBLIC cxx_std_20)
target_link_libraries(ufeel_cpp PUBLIC ufeel_wrapper Threads::Threads)
set_target_properties(ufeel_cpp PROPERTIES POSITION_INDEPENDENT_CODE ON)
