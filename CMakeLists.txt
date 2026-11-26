cmake_minimum_required(VERSION 3.24)

project(ufeel_detection_library LANGUAGES CXX)

if(NOT APPLE)
    add_link_options("-Wl,--disable-new-dtags")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

option(CLANG_TIDY_COLOR "Force colored clang-tidy diagnostics" ON)
option(ENABLE_CLANG_TIDY "Run clang-tidy as part of the compilation" OFF)
option(CLANG_TIDY_AS_ERRORS "Honour the WarningsAsErrors list in .clang-tidy" ON)
option(UFEEL_ENABLE_EMOTIONS "Build emotion detection support" ON)
option(UFEEL_ENABLE_EYE_TRACKING "Build eye tracking support" ON)
option(UFEEL_ENABLE_SPEECH_TO_TEXT "Build speech-to-text support" ON)
option(UFEEL_ENABLE_HEART_RATE "Build heart-rate support" ON)

if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)

    if(CLANG_TIDY_EXE)
        set(CLANG_TIDY_COMMAND
            "${CLANG_TIDY_EXE}"
            "--header-filter=^${CMAKE_SOURCE_DIR}/(Library|Wrapper)/"
            "--extra-arg=-Wno-unknown-warning-option"
        )

        if(CLANG_TIDY_COLOR)
            list(APPEND CLANG_TIDY_COMMAND "--use-color")
        endif()

        if(NOT CLANG_TIDY_AS_ERRORS)
            list(APPEND CLANG_TIDY_COMMAND "--warnings-as-errors=-*")
        endif()

        message(STATUS "clang-tidy enabled: ${CLANG_TIDY_EXE}")
    else()
        message(WARNING "clang-tidy requested but not found, skipping")
    endif()
endif()

set(THIRDPARTY_DIR "${CMAKE_SOURCE_DIR}/3rdparty")
set(LIBRARY_DIR "${CMAKE_SOURCE_DIR}/Library")
set(WRAPPER_DIR "${CMAKE_SOURCE_DIR}/Wrapper")
set(MODELS_DIR "${CMAKE_SOURCE_DIR}/models")

set(UFEEL_DEPS_PREFIX "" CACHE PATH "Install prefix of the dependencies built from the 3rdparty submodules by ./build.sh deps")

# The prebuilt libraries in 3rdparty/ are Linux x86-64 only. On macOS, ./build.sh deps downloads or
# builds them and links them into the dependencies prefix under the same directory names.
if(APPLE)
    set(PREBUILT_DIR "${UFEEL_DEPS_PREFIX}")
else()
    set(PREBUILT_DIR "${THIRDPARTY_DIR}")
endif()

set(OPENCV_DIR "${PREBUILT_DIR}/opencv")
set(LIBTORCH_DIR "${PREBUILT_DIR}/libtorch")
set(TFLITE_DIR "${PREBUILT_DIR}/tensorflowlite")
set(VOSK_DIR "${PREBUILT_DIR}/vosk")
set(JSON_DIR "${THIRDPARTY_DIR}/json")

set(PORTAUDIO_DIR "${UFEEL_DEPS_PREFIX}")
set(UFEEL_NEEDS_DEPS_PREFIX OFF)
if(APPLE OR UFEEL_ENABLE_EMOTIONS OR UFEEL_ENABLE_SPEECH_TO_TEXT OR UFEEL_ENABLE_HEART_RATE)
    set(UFEEL_NEEDS_DEPS_PREFIX ON)
endif()

if(UFEEL_NEEDS_DEPS_PREFIX AND NOT EXISTS "${UFEEL_DEPS_PREFIX}/.complete")
    message(FATAL_ERROR "UFEEL_DEPS_PREFIX ('${UFEEL_DEPS_PREFIX}') does not point to built dependencies. Run ./build.sh deps, or configure through ./build.sh")
endif()

set(SHARED_SUFFIX "${CMAKE_SHARED_LIBRARY_SUFFIX}")

set(CMAKE_BUILD_RPATH "${OPENCV_DIR}/lib")
if(UFEEL_ENABLE_EMOTIONS)
    list(APPEND CMAKE_BUILD_RPATH
        "${LIBTORCH_DIR}/lib"
        "${UFEEL_DEPS_PREFIX}/lib"
    )
endif()
if(UFEEL_ENABLE_SPEECH_TO_TEXT)
    list(APPEND CMAKE_BUILD_RPATH "${PORTAUDIO_DIR}/lib" "${VOSK_DIR}/lib")
endif()

set(UFEEL_REQUIRED_DIRS "${MODELS_DIR}")
list(APPEND UFEEL_REQUIRED_DIRS "${OPENCV_DIR}")
if(UFEEL_ENABLE_EMOTIONS OR UFEEL_ENABLE_EYE_TRACKING)
    list(APPEND UFEEL_REQUIRED_DIRS "${JSON_DIR}")
endif()
if(UFEEL_ENABLE_EMOTIONS)
    list(APPEND UFEEL_REQUIRED_DIRS "${LIBTORCH_DIR}")
endif()
if(UFEEL_ENABLE_EYE_TRACKING)
    list(APPEND UFEEL_REQUIRED_DIRS "${TFLITE_DIR}")
endif()
if(UFEEL_ENABLE_SPEECH_TO_TEXT)
    list(APPEND UFEEL_REQUIRED_DIRS "${VOSK_DIR}")
endif()

foreach(DIR IN LISTS UFEEL_REQUIRED_DIRS)
    if(NOT IS_DIRECTORY "${DIR}")
        message(FATAL_ERROR "Required directory not found: ${DIR}")
    endif()
endforeach()

if(UFEEL_ENABLE_EMOTIONS)
    set(Torch_DIR "${LIBTORCH_DIR}/share/cmake/Torch")
    find_package(Torch REQUIRED)
endif()

if(UFEEL_ENABLE_HEART_RATE)
    find_package(Threads REQUIRED)
    find_package(CURL CONFIG REQUIRED PATHS "${UFEEL_DEPS_PREFIX}" NO_DEFAULT_PATH)
    include("${UFEEL_DEPS_PREFIX}/lib/cmake/cpr/cprTargets.cmake")
endif()

if(APPLE)
    find_program(INSTALL_NAME_TOOL_EXECUTABLE NAMES install_name_tool REQUIRED)
    find_program(OTOOL_EXECUTABLE NAMES otool REQUIRED)
    find_program(CODESIGN_EXECUTABLE NAMES codesign REQUIRED)
else()
    find_program(PATCHELF_EXECUTABLE NAMES patchelf REQUIRED)
endif()

if(UFEEL_ENABLE_EMOTIONS)
    find_package(yaml-cpp 0.9 CONFIG REQUIRED PATHS "${UFEEL_DEPS_PREFIX}" NO_DEFAULT_PATH)
endif()

if(UFEEL_ENABLE_SPEECH_TO_TEXT)
    if(APPLE)
        set(PORTAUDIO_SONAME "libportaudio.2.dylib")
    else()
        find_path(PULSEAUDIO_INCLUDE_DIR pulse/pulseaudio.h REQUIRED)
        find_library(PULSEAUDIO_LIBRARY pulse REQUIRED)
        set(PORTAUDIO_SONAME "libportaudio.so.2")
    endif()
    file(REAL_PATH "${PORTAUDIO_DIR}/lib/${PORTAUDIO_SONAME}" PORTAUDIO_LIBRARY_FILE)
endif()

set(OPENCV_LIBRARIES
    "${OPENCV_DIR}/lib/libopencv_core${SHARED_SUFFIX}"
    "${OPENCV_DIR}/lib/libopencv_imgproc${SHARED_SUFFIX}"
    "${OPENCV_DIR}/lib/libopencv_imgcodecs${SHARED_SUFFIX}"
    "${OPENCV_DIR}/lib/libopencv_dnn${SHARED_SUFFIX}"
    "${OPENCV_DIR}/lib/libopencv_objdetect${SHARED_SUFFIX}"
    "${OPENCV_DIR}/lib/libopencv_videoio${SHARED_SUFFIX}"
)

file(GLOB_RECURSE TFLITE_DEPENDENCY_LIBRARIES
    CONFIGURE_DEPENDS
    "${TFLITE_DIR}/lib/deps/*.a"
)

set(TFLITE_STATIC_LIBRARIES
    "${TFLITE_DIR}/lib/libtensorflow-lite.a"
    ${TFLITE_DEPENDENCY_LIBRARIES}
    "${LIBTORCH_DIR}/lib/libpthreadpool.a"
    # ! required by tflite, provided by libtorch
    # TODO: find a better way to handle this dependency, maybe by building tflite with pthreadpool support disabled
)

# ld64 searches static libraries repeatedly, so only the other linkers need a link group.
if(APPLE)
    set(EYE_TRACKING_LINK_LIBRARIES ufeel_feature_eye_tracking ${TFLITE_STATIC_LIBRARIES})
else()
    set(EYE_TRACKING_LINK_LIBRARIES
        "$<LINK_GROUP:RESCAN,ufeel_feature_eye_tracking,${TFLITE_STATIC_LIBRARIES}>"
    )
endif()

set(VOSK_KALDI_LIBRARIES
    "${VOSK_DIR}/lib/kaldi/online2/kaldi-online2.a"
    "${VOSK_DIR}/lib/kaldi/decoder/kaldi-decoder.a"
    "${VOSK_DIR}/lib/kaldi/ivector/kaldi-ivector.a"
    "${VOSK_DIR}/lib/kaldi/gmm/kaldi-gmm.a"
    "${VOSK_DIR}/lib/kaldi/tree/kaldi-tree.a"
    "${VOSK_DIR}/lib/kaldi/feat/kaldi-feat.a"
    "${VOSK_DIR}/lib/kaldi/lat/kaldi-lat.a"
    "${VOSK_DIR}/lib/kaldi/lm/kaldi-lm.a"
    "${VOSK_DIR}/lib/kaldi/rnnlm/kaldi-rnnlm.a"
    "${VOSK_DIR}/lib/kaldi/hmm/kaldi-hmm.a"
    "${VOSK_DIR}/lib/kaldi/nnet3/kaldi-nnet3.a"
    "${VOSK_DIR}/lib/kaldi/transform/kaldi-transform.a"
    "${VOSK_DIR}/lib/kaldi/cudamatrix/kaldi-cudamatrix.a"
    "${VOSK_DIR}/lib/kaldi/matrix/kaldi-matrix.a"
    "${VOSK_DIR}/lib/kaldi/fstext/kaldi-fstext.a"
    "${VOSK_DIR}/lib/kaldi/util/kaldi-util.a"
    "${VOSK_DIR}/lib/kaldi/base/kaldi-base.a"
    "${VOSK_DIR}/lib/openfst/libfst.a"
    "${VOSK_DIR}/lib/openfst/libfstngram.a"
    "${VOSK_DIR}/lib/openblas/liblapack.a"
    "${VOSK_DIR}/lib/openblas/libf2c.a"
    "${VOSK_DIR}/lib/openblas/libblas.a"
    "${VOSK_DIR}/lib/openblas/libopenblas-r0.3.20.a"
)

if(UFEEL_ENABLE_EMOTIONS)
    add_library(ufeel_feature_emotions STATIC
        ${LIBRARY_DIR}/src/Emotions/ResNet.cpp
        ${LIBRARY_DIR}/src/Emotions/EmotionModel.cpp
        ${LIBRARY_DIR}/src/Emotions/CheckPointLoader.cpp
        ${LIBRARY_DIR}/src/Emotions/Inference/HeadCalibration.cpp
        ${LIBRARY_DIR}/src/Emotions/Inference/FaceDetector.cpp
        ${LIBRARY_DIR}/src/Emotions/Inference/FaceCrop.cpp
        ${LIBRARY_DIR}/src/Emotions/Inference/InferencePipeline.cpp
        ${LIBRARY_DIR}/src/Emotions/EmotionDetector.cpp
    )
endif()

if(UFEEL_ENABLE_EYE_TRACKING)
    add_library(ufeel_feature_eye_tracking STATIC
        ${LIBRARY_DIR}/src/EyeTracking/IrisDetection/DetectionPostProcess.cpp
        ${LIBRARY_DIR}/src/EyeTracking/IrisDetection/FaceDetection.cpp
        ${LIBRARY_DIR}/src/EyeTracking/IrisDetection/FaceLandmark.cpp
        ${LIBRARY_DIR}/src/EyeTracking/IrisDetection/IrisLandmark.cpp
        ${LIBRARY_DIR}/src/EyeTracking/IrisDetection/ModelLoader.cpp
        ${LIBRARY_DIR}/src/EyeTracking/EyeBlinkTracker.cpp
        ${LIBRARY_DIR}/src/EyeTracking/EyeGazeDirection.cpp
        ${LIBRARY_DIR}/src/EyeTracking/EyeTrackingCalibrationModel.cpp
        ${LIBRARY_DIR}/src/EyeTracking/EyeTrackingDetector.cpp
        ${LIBRARY_DIR}/src/EyeTracking/EyeTrackingKalmanFilter.cpp
    )
endif()

if(UFEEL_ENABLE_SPEECH_TO_TEXT)
    add_library(ufeel_feature_speech_to_text STATIC
        ${LIBRARY_DIR}/src/SpeechToText/AudioCapture.cpp
        ${LIBRARY_DIR}/src/SpeechToText/SpeechRecognizer.cpp
        ${LIBRARY_DIR}/src/SpeechToText/SpeechToTextDetector.cpp
    )
    if(APPLE)
        target_sources(ufeel_feature_speech_to_text PRIVATE
            ${LIBRARY_DIR}/src/SpeechToText/CoreAudioDevices.cpp
        )
        target_link_libraries(ufeel_feature_speech_to_text PRIVATE "-framework CoreAudio")
    else()
        target_sources(ufeel_feature_speech_to_text PRIVATE
            ${LIBRARY_DIR}/src/SpeechToText/PulseSources.cpp
        )
    endif()
endif()

if(UFEEL_ENABLE_HEART_RATE)
    add_library(ufeel_feature_heart_rate STATIC
        ${LIBRARY_DIR}/src/HeartRate/HeartRateDetector.cpp
        ${LIBRARY_DIR}/src/HeartRate/HeartRateSource.cpp
        ${LIBRARY_DIR}/src/HeartRate/SensorHeartRateSource.cpp
        ${LIBRARY_DIR}/src/HeartRate/WatchHeartRateSource.cpp
    )
endif()

add_library(ufeel_detection_library STATIC
    ${LIBRARY_DIR}/src/DataProcessor.cpp
)

set(UFEEL_FEATURE_TARGETS)
if(UFEEL_ENABLE_EMOTIONS)
    list(APPEND UFEEL_FEATURE_TARGETS ufeel_feature_emotions)
endif()
if(UFEEL_ENABLE_EYE_TRACKING)
    list(APPEND UFEEL_FEATURE_TARGETS ufeel_feature_eye_tracking)
endif()
if(UFEEL_ENABLE_SPEECH_TO_TEXT)
    list(APPEND UFEEL_FEATURE_TARGETS ufeel_feature_speech_to_text)
endif()
if(UFEEL_ENABLE_HEART_RATE)
    list(APPEND UFEEL_FEATURE_TARGETS ufeel_feature_heart_rate)
endif()

foreach(FEATURE_TARGET IN LISTS UFEEL_FEATURE_TARGETS)
    target_include_directories(${FEATURE_TARGET}
        PUBLIC
            "${LIBRARY_DIR}/include"
    )
    target_include_directories(${FEATURE_TARGET}
        SYSTEM PUBLIC
            "${OPENCV_DIR}/include/opencv4"
            ${TORCH_INCLUDE_DIRS}
            "${TFLITE_DIR}/include"
            "${VOSK_DIR}/include"
            "${PORTAUDIO_DIR}/include"
            "${JSON_DIR}/include"
    )
    target_compile_options(${FEATURE_TARGET} PRIVATE ${TORCH_CXX_FLAGS})
endforeach()

target_include_directories(ufeel_detection_library
    PUBLIC
        "${LIBRARY_DIR}/include"
)

target_include_directories(ufeel_detection_library
    SYSTEM PUBLIC
        "${OPENCV_DIR}/include/opencv4"
        ${TORCH_INCLUDE_DIRS}
        "${TFLITE_DIR}/include"
        "${VOSK_DIR}/include"
        "${PORTAUDIO_DIR}/include"
        "${JSON_DIR}/include"
)

target_link_libraries(ufeel_detection_library
    PUBLIC
        "${OPENCV_DIR}/lib/libopencv_core${SHARED_SUFFIX}"
        "${OPENCV_DIR}/lib/libopencv_videoio${SHARED_SUFFIX}"
        "${OPENCV_DIR}/lib/libopencv_imgproc${SHARED_SUFFIX}"
        $<$<BOOL:${UFEEL_ENABLE_EMOTIONS}>:ufeel_feature_emotions>
        $<$<BOOL:${UFEEL_ENABLE_EYE_TRACKING}>:ufeel_feature_eye_tracking>
        $<$<BOOL:${UFEEL_ENABLE_SPEECH_TO_TEXT}>:ufeel_feature_speech_to_text>
        $<$<BOOL:${UFEEL_ENABLE_HEART_RATE}>:ufeel_feature_heart_rate>
)

target_compile_options(ufeel_detection_library
    PRIVATE
        ${TORCH_CXX_FLAGS}
)

target_compile_definitions(ufeel_detection_library
    PUBLIC
        UFEEL_ENABLE_EMOTIONS=$<BOOL:${UFEEL_ENABLE_EMOTIONS}>
        UFEEL_ENABLE_EYE_TRACKING=$<BOOL:${UFEEL_ENABLE_EYE_TRACKING}>
        UFEEL_ENABLE_SPEECH_TO_TEXT=$<BOOL:${UFEEL_ENABLE_SPEECH_TO_TEXT}>
        UFEEL_ENABLE_HEART_RATE=$<BOOL:${UFEEL_ENABLE_HEART_RATE}>
)

if(UFEEL_ENABLE_EMOTIONS)
    target_link_libraries(ufeel_feature_emotions
        PUBLIC
            ${OPENCV_LIBRARIES}
            ${TORCH_LIBRARIES}
            yaml-cpp::yaml-cpp
    )
endif()

if(UFEEL_ENABLE_EYE_TRACKING)
    target_link_libraries(ufeel_feature_eye_tracking PUBLIC ${OPENCV_LIBRARIES})
endif()

if(UFEEL_ENABLE_SPEECH_TO_TEXT)
    if(APPLE)
        target_link_libraries(ufeel_feature_speech_to_text
            PUBLIC
                "${VOSK_DIR}/lib/libvosk.dylib"
                "${PORTAUDIO_DIR}/lib/libportaudio.dylib"
        )
    else()
        target_link_libraries(ufeel_feature_speech_to_text
            PUBLIC
                "-Wl,--start-group"
                "${VOSK_DIR}/lib/libvosk.a"
                ${VOSK_KALDI_LIBRARIES}
                "-Wl,--end-group"
                "${PORTAUDIO_DIR}/lib/libportaudio.so"
            PRIVATE
                "${PULSEAUDIO_LIBRARY}"
        )
        target_include_directories(ufeel_feature_speech_to_text SYSTEM PRIVATE "${PULSEAUDIO_INCLUDE_DIR}")
    endif()
endif()

if(UFEEL_ENABLE_HEART_RATE)
    target_link_libraries(ufeel_feature_heart_rate
        PUBLIC
            cpr::cpr
    )
endif()

option(UFEEL_ENABLE_COVERAGE "Enable LLVM source-based coverage" OFF)

if(UFEEL_ENABLE_COVERAGE)
    target_compile_options(ufeel_detection_library PRIVATE
        -fprofile-instr-generate
        -fcoverage-mapping
    )

    target_link_options(ufeel_detection_library INTERFACE
        -fprofile-instr-generate
    )

    foreach(FEATURE_TARGET IN LISTS UFEEL_FEATURE_TARGETS)
        target_compile_options(${FEATURE_TARGET} PRIVATE
            -fprofile-instr-generate
            -fcoverage-mapping
        )
    endforeach()

endif()

add_subdirectory(Wrapper)

if(UFEEL_ENABLE_COVERAGE)
    target_compile_options(ufeel_wrapper PRIVATE
        -fprofile-instr-generate
        -fcoverage-mapping
    )

    target_link_options(ufeel_wrapper PRIVATE
        -fprofile-instr-generate
    )
endif()

if(CLANG_TIDY_COMMAND)
    set_target_properties(
        ufeel_detection_library
        ufeel_wrapper
        PROPERTIES
            CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}"
    )
endif()

function(ufeel_install_opencv_library MODULE)
    if(APPLE)
        install(
            FILES "${OPENCV_DIR}/lib/libopencv_${MODULE}.4.8.1.dylib"
            RENAME "libopencv_${MODULE}.408.dylib"
            DESTINATION dependencies
        )
    else()
        install(
            FILES "${OPENCV_DIR}/lib/libopencv_${MODULE}.so.4.8.1"
            RENAME "libopencv_${MODULE}.so.408"
            DESTINATION dependencies
        )
    endif()
endfunction()

foreach(MODULE IN ITEMS core videoio imgproc imgcodecs)
    ufeel_install_opencv_library(${MODULE})
endforeach()

if(UFEEL_ENABLE_EMOTIONS OR UFEEL_ENABLE_EYE_TRACKING)
    foreach(MODULE IN ITEMS dnn objdetect calib3d features2d flann)
        ufeel_install_opencv_library(${MODULE})
    endforeach()
endif()

if(UFEEL_ENABLE_EMOTIONS)
    install(
        FILES
            "${LIBTORCH_DIR}/lib/libtorch${SHARED_SUFFIX}"
            "${LIBTORCH_DIR}/lib/libtorch_cpu${SHARED_SUFFIX}"
            "${LIBTORCH_DIR}/lib/libc10${SHARED_SUFFIX}"
        DESTINATION dependencies
    )
    if(APPLE)
        install(
            FILES "${LIBTORCH_DIR}/lib/libomp.dylib"
            DESTINATION dependencies
        )
    else()
        install(
            FILES "${LIBTORCH_DIR}/lib/libgomp-98b21ff3.so.1"
            DESTINATION dependencies
        )
    endif()
endif()

if(UFEEL_ENABLE_SPEECH_TO_TEXT)
    install(
        FILES "${PORTAUDIO_LIBRARY_FILE}"
        RENAME ${PORTAUDIO_SONAME}
        DESTINATION dependencies
    )
    if(APPLE)
        install(
            FILES "${VOSK_DIR}/lib/libvosk.dylib"
            DESTINATION dependencies
        )
    endif()
endif()

if(UFEEL_ENABLE_EMOTIONS)
    if(APPLE)
        file(GLOB YAML_CPP_SHARED_LIBRARIES "${UFEEL_DEPS_PREFIX}/lib/libyaml-cpp*.dylib")
    else()
        file(GLOB YAML_CPP_SHARED_LIBRARIES "${UFEEL_DEPS_PREFIX}/lib/libyaml-cpp.so*")
    endif()
    install(
        FILES ${YAML_CPP_SHARED_LIBRARIES}
        DESTINATION dependencies
    )
endif()

set(WRAPPER_FILE_NAME "${CMAKE_SHARED_LIBRARY_PREFIX}ufeel_wrapper${SHARED_SUFFIX}")

if(APPLE)
    set(CONFIGURE_DEPENDENCIES_TEMPLATE "ConfigureDependenciesMacOS.cmake.in")
else()
    set(CONFIGURE_DEPENDENCIES_TEMPLATE "ConfigureDependencies.cmake.in")
endif()

configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/${CONFIGURE_DEPENDENCIES_TEMPLATE}"
    "${CMAKE_CURRENT_BINARY_DIR}/ConfigureDependencies.cmake"
    @ONLY
)

install(
    SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/ConfigureDependencies.cmake"
)

if(UFEEL_ENABLE_EMOTIONS)
    install(DIRECTORY "${MODELS_DIR}/Emotions/" DESTINATION models/Emotions)
endif()
if(UFEEL_ENABLE_EYE_TRACKING)
    install(DIRECTORY "${MODELS_DIR}/EyeTracking/" DESTINATION models/EyeTracking)
endif()
if(UFEEL_ENABLE_SPEECH_TO_TEXT)
    install(DIRECTORY "${MODELS_DIR}/SpeechToText/" DESTINATION models/SpeechToText)
endif()

add_subdirectory(Adapters)

option(UFEEL_BUILD_TESTS "Build unit tests" ON)

if(UFEEL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# TODO: improve device discovery logic

# TODO: remove the 'k' usage for constants -> find a cleaner solution

# TODO: bundle package back into a Linux/x86_64 subdirectory
