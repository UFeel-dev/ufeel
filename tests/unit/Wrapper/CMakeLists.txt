add_executable(
    run_wrapper_unit_tests

    src/TestEmotions.cpp
    src/TestEyeTracking.cpp
    src/TestHeartRate.cpp
    src/TestProcessor.cpp
    src/TestSpeechToText.cpp
    src/TestApiIntegration.cpp
    src/TestWrapperBehavior.cpp

    src/internal/TestErrorHandling.cpp
    src/internal/TestFrameConversion.cpp
    src/internal/TestMemory.cpp
)

target_include_directories(run_wrapper_unit_tests PRIVATE
    "${WRAPPER_DIR}/include"
    "${WRAPPER_DIR}/src/include"
    "${LIBRARY_DIR}/include"
)

target_include_directories(run_wrapper_unit_tests SYSTEM PRIVATE
    "${OPENCV_DIR}/include/opencv4"
    "${TFLITE_DIR}/include"
)

target_compile_definitions(run_wrapper_unit_tests PRIVATE
    UFEEL_TEST_MODEL_DIR="${CMAKE_SOURCE_DIR}/models"
    UFEEL_TEST_SPEECH_MODEL_DIR="${CMAKE_SOURCE_DIR}/models/SpeechToText/vosk-model-small-en-us-0.15"
)

target_link_libraries(
    run_wrapper_unit_tests
    PRIVATE
        GTest::gtest_main
        ufeel_wrapper
        "${OPENCV_DIR}/lib/libopencv_core.so"
)

include(GoogleTest)

gtest_discover_tests(
    run_wrapper_unit_tests
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

if(UFEEL_ENABLE_COVERAGE)
    target_compile_options(run_wrapper_unit_tests PRIVATE
        -fprofile-instr-generate
        -fcoverage-mapping
    )

    target_link_options(run_wrapper_unit_tests PRIVATE
        -fprofile-instr-generate
    )
endif()
