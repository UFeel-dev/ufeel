if(UFEEL_ENABLE_EMOTIONS AND UFEEL_ENABLE_EYE_TRACKING AND
   UFEEL_ENABLE_SPEECH_TO_TEXT AND UFEEL_ENABLE_HEART_RATE)

add_executable(
    run_wrapper_e2e_tests

    Wrapper/TestApiIntegration.cpp
)

target_include_directories(run_wrapper_e2e_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../unit/cpp/include"
    "${WRAPPER_DIR}/include"
    "${WRAPPER_DIR}/src/include"
    "${LIBRARY_DIR}/include"
)

target_include_directories(run_wrapper_e2e_tests SYSTEM PRIVATE
    "${OPENCV_DIR}/include/opencv4"
    "${TFLITE_DIR}/include"
)

target_compile_definitions(run_wrapper_e2e_tests PRIVATE
    UFEEL_TEST_MODEL_DIR="${CMAKE_SOURCE_DIR}/models"
    UFEEL_TEST_SPEECH_MODEL_DIR="${CMAKE_SOURCE_DIR}/models/SpeechToText/vosk-model-small-en-us-0.15"
)

target_link_libraries(
    run_wrapper_e2e_tests
    PRIVATE
        GTest::gtest_main
        ufeel_wrapper
        "${OPENCV_DIR}/lib/libopencv_core.so"
)

include(GoogleTest)

gtest_discover_tests(
    run_wrapper_e2e_tests
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    PROPERTIES LABELS "e2e\;hardware"
    DISCOVERY_MODE PRE_TEST
)

endif()
