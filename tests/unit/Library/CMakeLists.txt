add_executable(
    run_library_unit_tests

    Utils/TestFrameChecker.cpp
)

if(NOT UFEEL_ENABLE_EMOTIONS AND NOT UFEEL_ENABLE_EYE_TRACKING AND
   NOT UFEEL_ENABLE_SPEECH_TO_TEXT AND NOT UFEEL_ENABLE_HEART_RATE)
    target_sources(run_library_unit_tests PRIVATE TestFeatureSelection.cpp)
endif()

if(UFEEL_ENABLE_EMOTIONS AND UFEEL_ENABLE_EYE_TRACKING AND
   UFEEL_ENABLE_SPEECH_TO_TEXT AND UFEEL_ENABLE_HEART_RATE)
    target_sources(run_library_unit_tests PRIVATE TestDataProcessor.cpp)
endif()

if(UFEEL_ENABLE_EMOTIONS)
    target_sources(run_library_unit_tests PRIVATE
        Emotions/TestEmotionModel.cpp
        Emotions/TestEmotionDetector.cpp
        Emotions/TestHeadCalibration.cpp
        Emotions/TestResNet.cpp
        Emotions/TestCheckPointLoader.cpp
        Emotions/Inference/TestFaceCrop.cpp
        Emotions/Inference/TestFaceDetector.cpp
        Emotions/Inference/TestInferencePipeline.cpp
    )
endif()

if(UFEEL_ENABLE_EYE_TRACKING)
    target_sources(run_library_unit_tests PRIVATE
        EyeTracking/IrisDetection/TestDetectionPostProcess.cpp
        EyeTracking/IrisDetection/TestModelLoader.cpp
        EyeTracking/IrisDetection/TestFaceDetection.cpp
        EyeTracking/IrisDetection/TestFaceLandmark.cpp
        EyeTracking/IrisDetection/TestIrisLandmark.cpp
        EyeTracking/TestEyeTrackingCalibrationModel.cpp
        EyeTracking/TestEyeTrackingDetector.cpp
        EyeTracking/TestEyeTrackingKalmanFilter.cpp
    )
endif()

if(UFEEL_ENABLE_SPEECH_TO_TEXT)
    target_sources(run_library_unit_tests PRIVATE
        SpeechToText/TestAudioCapture.cpp
        SpeechToText/TestSpeechRecognizer.cpp
        SpeechToText/TestSpeechToTextDetector.cpp
    )
endif()

if(UFEEL_ENABLE_HEART_RATE)
    target_sources(run_library_unit_tests PRIVATE
        HeartRate/TestHeartRateDetector.cpp
        HeartRate/TestHeartRateSource.cpp
        HeartRate/TestSensorHeartRateSource.cpp
        HeartRate/TestWatchHeartRateSource.cpp
    )
endif()

target_link_libraries(
    run_library_unit_tests
    PRIVATE
        GTest::gtest_main
        ufeel_detection_library
)

if(UFEEL_ENABLE_EYE_TRACKING)
    target_link_libraries(
        run_library_unit_tests
        PRIVATE
            "$<LINK_GROUP:RESCAN,ufeel_feature_eye_tracking,${TFLITE_STATIC_LIBRARIES}>"
    )
endif()

target_include_directories(run_library_unit_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../include"
    ${TFLITE_DIR}/include
)

target_compile_definitions(run_library_unit_tests PRIVATE
    DATA_PROCESSOR_MODEL_DIR_PATH="${CMAKE_SOURCE_DIR}/models"
    EMOTIONS_MODEL_DIR_PATH="${CMAKE_SOURCE_DIR}/models"
    MODEL_LOADER_TEST_MODEL_PATH="${CMAKE_SOURCE_DIR}/models/EyeTracking/face_detection_short.tflite"
    MODEL_DIR_PATH="${CMAKE_SOURCE_DIR}/models/EyeTracking"
    RESOURCES_DIR_PATH="${CMAKE_SOURCE_DIR}/tests/resources"
)

include(GoogleTest)

gtest_discover_tests(
    run_library_unit_tests
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    DISCOVERY_MODE PRE_TEST
)

if(UFEEL_ENABLE_COVERAGE)
    target_compile_options(run_library_unit_tests PRIVATE
        -fprofile-instr-generate
        -fcoverage-mapping
    )

    target_link_options(run_library_unit_tests PRIVATE
        -fprofile-instr-generate
    )
endif()
