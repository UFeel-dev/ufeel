add_executable(
    run_feature_loading_tests

    Wrapper/TestFeatureLoading.cpp
)

add_dependencies(run_feature_loading_tests ufeel_wrapper)

target_include_directories(run_feature_loading_tests PRIVATE "${WRAPPER_DIR}/include")

target_compile_definitions(run_feature_loading_tests PRIVATE
    UFEEL_TEST_WRAPPER_PATH="$<TARGET_FILE:ufeel_wrapper>"
    UFEEL_TEST_MODEL_DIR="${CMAKE_SOURCE_DIR}/models"
    UFEEL_TEST_SPEECH_MODEL_DIR="${CMAKE_SOURCE_DIR}/models/SpeechToText/vosk-model-small-en-us-0.15"
    UFEEL_ENABLE_EMOTIONS=$<BOOL:${UFEEL_ENABLE_EMOTIONS}>
    UFEEL_ENABLE_EYE_TRACKING=$<BOOL:${UFEEL_ENABLE_EYE_TRACKING}>
    UFEEL_ENABLE_SPEECH_TO_TEXT=$<BOOL:${UFEEL_ENABLE_SPEECH_TO_TEXT}>
    UFEEL_ENABLE_HEART_RATE=$<BOOL:${UFEEL_ENABLE_HEART_RATE}>
)

target_link_libraries(run_feature_loading_tests PRIVATE GTest::gtest_main ${CMAKE_DL_LIBS})

include(GoogleTest)

gtest_discover_tests(
    run_feature_loading_tests
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    PROPERTIES LABELS integration
    DISCOVERY_MODE PRE_TEST
)

if(UFEEL_ENABLE_EMOTIONS AND UFEEL_ENABLE_EYE_TRACKING AND
   UFEEL_ENABLE_SPEECH_TO_TEXT AND UFEEL_ENABLE_HEART_RATE)

add_executable(
    run_library_integration_tests

    Library/TestDataProcessor.cpp
)

target_include_directories(run_library_integration_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../unit/cpp/include"
)

target_include_directories(run_library_integration_tests SYSTEM PRIVATE
    "${TFLITE_DIR}/include"
)

target_compile_definitions(run_library_integration_tests PRIVATE
    UFEEL_TEST_MODEL_DIR="${CMAKE_SOURCE_DIR}/models"
    UFEEL_TEST_SPEECH_MODEL_DIR="${CMAKE_SOURCE_DIR}/models/SpeechToText/vosk-model-small-en-us-0.15"
    RESOURCES_DIR_PATH="${CMAKE_SOURCE_DIR}/tests/resources"
)

target_link_libraries(
    run_library_integration_tests
    PRIVATE
        GTest::gtest_main
        ufeel_detection_library
        "$<LINK_GROUP:RESCAN,ufeel_feature_eye_tracking,${TFLITE_STATIC_LIBRARIES}>"
)

add_executable(
    run_wrapper_integration_tests

    Wrapper/TestProcessor.cpp
)

target_include_directories(run_wrapper_integration_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../unit/cpp/include"
    "${WRAPPER_DIR}/include"
    "${WRAPPER_DIR}/src/include"
    "${LIBRARY_DIR}/include"
)

target_include_directories(run_wrapper_integration_tests SYSTEM PRIVATE
    "${OPENCV_DIR}/include/opencv4"
    "${TFLITE_DIR}/include"
)

target_link_libraries(
    run_wrapper_integration_tests
    PRIVATE
        GTest::gtest_main
        ufeel_wrapper
        "${OPENCV_DIR}/lib/libopencv_core.so"
)

include(GoogleTest)

gtest_discover_tests(
    run_library_integration_tests
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    PROPERTIES LABELS integration
    DISCOVERY_MODE PRE_TEST
)

gtest_discover_tests(
    run_wrapper_integration_tests
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    PROPERTIES LABELS integration
    DISCOVERY_MODE PRE_TEST
)

endif()
