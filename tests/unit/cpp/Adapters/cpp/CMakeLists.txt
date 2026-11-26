if(NOT TARGET ufeel_cpp)
    add_subdirectory("${CMAKE_SOURCE_DIR}/Adapters/cpp/UFeel" "${CMAKE_BINARY_DIR}/Adapters/cpp/UFeel")
endif()

add_executable(
    run_cpp_adapter_unit_tests

    Calibration/TestCalibrationResult.cpp
    Calibration/TestEyeTrackingCalibration.cpp
    Configuration/TestCalibrationProfileStore.cpp
    Configuration/TestPackageLayout.cpp
    Model/TestDetection.cpp
    Model/TestEmotionResult.cpp
    Model/TestStatus.cpp
    Native/TestError.cpp
    Native/TestProcessor.cpp
    Pipeline/TestEyeTrackingSmoother.cpp
    Pipeline/TestFeatureDemand.cpp
    Pipeline/TestFeatureScheduler.cpp
    Pipeline/TestLatestFrameCapture.cpp
    Pipeline/TestSession.cpp
    Rules/TestRuleSet.cpp
    Text/TestText.cpp
)

target_include_directories(run_cpp_adapter_unit_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
    "${WRAPPER_DIR}/src/include"
    "${LIBRARY_DIR}/include"
)

target_include_directories(run_cpp_adapter_unit_tests SYSTEM PRIVATE
    "${OPENCV_DIR}/include/opencv4"
    "${TFLITE_DIR}/include"
)

target_link_libraries(
    run_cpp_adapter_unit_tests
    PRIVATE
        GTest::gtest_main
        ufeel_cpp
        "${OPENCV_DIR}/lib/libopencv_core.so"
)

include(GoogleTest)

gtest_discover_tests(
    run_cpp_adapter_unit_tests
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    PROPERTIES LABELS unit
    DISCOVERY_MODE PRE_TEST
)

if(UFEEL_ENABLE_COVERAGE)
    foreach(target IN ITEMS ufeel_cpp run_cpp_adapter_unit_tests)
        target_compile_options(${target} PRIVATE -fprofile-instr-generate -fcoverage-mapping)
    endforeach()

    target_link_options(run_cpp_adapter_unit_tests PRIVATE -fprofile-instr-generate)
endif()
