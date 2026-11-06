add_executable(
    run_unit_tests
    Emotions/TestEmotionModel.cpp
    Emotions/TestEmotionDetector.cpp
    Emotions/TestHeadCalibration.cpp
    Emotions/TestResNet.cpp
    Emotions/TestCheckPointLoader.cpp

    Emotions/Inference/TestFaceCrop.cpp
    Emotions/Inference/TestFaceDetector.cpp
    Emotions/Inference/TestInferencePipeline.cpp

    EyeTracking/IrisDetection/TestDetectionPostProcess.cpp
    EyeTracking/IrisDetection/TestModelLoader.cpp
    EyeTracking/IrisDetection/TestFaceDetection.cpp
    EyeTracking/IrisDetection/TestFaceLandmark.cpp

    SpeechToText/TestAudioCapture.cpp
    SpeechToText/TestSpeechRecognizer.cpp
    SpeechToText/TestSpeechToTextDetector.cpp

    HeartRate/TestHeartRateDetector.cpp
)

target_link_libraries(
    run_unit_tests
    PRIVATE
        GTest::gtest_main
        ufeel_detection_library
)

target_include_directories(run_unit_tests PRIVATE
    ${TFLITE_DIR}/include
)

target_compile_definitions(run_unit_tests PRIVATE
    EMOTIONS_MODEL_DIR_PATH="${CMAKE_SOURCE_DIR}/models"
    MODEL_LOADER_TEST_MODEL_PATH="${CMAKE_SOURCE_DIR}/models/EyeTracking/face_detection_short.tflite"
    MODEL_DIR_PATH="${CMAKE_SOURCE_DIR}/models/EyeTracking"
)

include(GoogleTest)

gtest_discover_tests(
    run_unit_tests
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

if(UFEEL_ENABLE_COVERAGE)
    target_compile_options(run_unit_tests PRIVATE
        -fprofile-instr-generate
        -fcoverage-mapping
    )

    target_link_options(run_unit_tests PRIVATE
        -fprofile-instr-generate
    )
endif()
