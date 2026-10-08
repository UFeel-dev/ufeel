cmake_minimum_required(VERSION 3.20)
project(ufeel_detection_library)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(CLANG_TIDY_COLOR "Force colored clang-tidy diagnostics" ON)
option(ENABLE_CLANG_TIDY "Run clang-tidy as part of the compilation" OFF)
option(CLANG_TIDY_AS_ERRORS "Honour the WarningsAsErrors list in .clang-tidy" ON)

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

set(THIRDPARTY_DIR ${CMAKE_SOURCE_DIR}/3rdparty)
set(LIBRARY_DIR ${CMAKE_SOURCE_DIR}/Library)
set(WRAPPER_DIR ${CMAKE_SOURCE_DIR}/Wrapper)

set(MODELS_DIR "${CMAKE_SOURCE_DIR}/models")

if(NOT IS_DIRECTORY "${MODELS_DIR}")
    message(FATAL_ERROR "Models directory not found: ${MODELS_DIR}")
endif()

set(OPENCV_DIR ${THIRDPARTY_DIR}/opencv)
set(LIBTORCH_DIR ${THIRDPARTY_DIR}/libtorch)
set(TFLITE_DIR ${THIRDPARTY_DIR}/tensorflowlite)
set(VOSK_DIR ${THIRDPARTY_DIR}/vosk)
set(PORTAUDIO_DIR ${THIRDPARTY_DIR}/portaudio)

set(Torch_DIR ${LIBTORCH_DIR}/share/cmake/Torch)
find_package(Torch REQUIRED)

add_library(ufeel_detection_library STATIC
    ${LIBRARY_DIR}/src/DataProcessor.cpp

    ${LIBRARY_DIR}/src/Emotions/CheckPointLoader.cpp
    ${LIBRARY_DIR}/src/Emotions/ResNetImpl.cpp
    ${LIBRARY_DIR}/src/Emotions/Model.cpp
    ${LIBRARY_DIR}/src/Emotions/EmotionDetector.cpp

    ${LIBRARY_DIR}/src/EyeTracking/IrisDetection/DetectionPostProcess.cpp
    ${LIBRARY_DIR}/src/EyeTracking/IrisDetection/FaceDetection.cpp
    ${LIBRARY_DIR}/src/EyeTracking/IrisDetection/FaceLandmark.cpp
    ${LIBRARY_DIR}/src/EyeTracking/IrisDetection/IrisLandmark.cpp
    ${LIBRARY_DIR}/src/EyeTracking/IrisDetection/ModelLoader.cpp
    ${LIBRARY_DIR}/src/EyeTracking/EyeTrackingDetector.cpp

    ${LIBRARY_DIR}/src/SpeechToText/AudioCapture.cpp
    ${LIBRARY_DIR}/src/SpeechToText/SpeechRecognizer.cpp
    ${LIBRARY_DIR}/src/SpeechToText/SpeechToTextDetector.cpp
)

if(CLANG_TIDY_COMMAND)
    set_target_properties(ufeel_detection_library PROPERTIES
        CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}"
    )
endif()

target_include_directories(ufeel_detection_library
    PUBLIC
        ${LIBRARY_DIR}/include
    SYSTEM PUBLIC
        ${OPENCV_DIR}/include/opencv4
        ${TORCH_INCLUDE_DIRS}
        ${TFLITE_DIR}/include
        ${VOSK_DIR}/include
        ${PORTAUDIO_DIR}/include
)

target_link_libraries(ufeel_detection_library PUBLIC
    ${OPENCV_DIR}/lib/libopencv_core.so
    ${OPENCV_DIR}/lib/libopencv_imgproc.so
    ${OPENCV_DIR}/lib/libopencv_imgcodecs.so
    ${OPENCV_DIR}/lib/libopencv_dnn.so
    ${OPENCV_DIR}/lib/libopencv_objdetect.so
    ${OPENCV_DIR}/lib/libopencv_videoio.so

    ${TORCH_LIBRARIES}

    ${TFLITE_DIR}/lib/libtensorflow-lite.a
    ${TFLITE_DIR}/lib/flatbuffers/libflatbuffers.a
    ${TFLITE_DIR}/lib/ruy/libruy_combined.a

    ${VOSK_DIR}/lib/libvosk.a

    -Wl,--start-group
    ${VOSK_DIR}/lib/kaldi/online2/kaldi-online2.a
    ${VOSK_DIR}/lib/kaldi/decoder/kaldi-decoder.a
    ${VOSK_DIR}/lib/kaldi/ivector/kaldi-ivector.a
    ${VOSK_DIR}/lib/kaldi/gmm/kaldi-gmm.a
    ${VOSK_DIR}/lib/kaldi/tree/kaldi-tree.a
    ${VOSK_DIR}/lib/kaldi/feat/kaldi-feat.a
    ${VOSK_DIR}/lib/kaldi/lat/kaldi-lat.a
    ${VOSK_DIR}/lib/kaldi/lm/kaldi-lm.a
    ${VOSK_DIR}/lib/kaldi/rnnlm/kaldi-rnnlm.a
    ${VOSK_DIR}/lib/kaldi/hmm/kaldi-hmm.a
    ${VOSK_DIR}/lib/kaldi/nnet3/kaldi-nnet3.a
    ${VOSK_DIR}/lib/kaldi/transform/kaldi-transform.a
    ${VOSK_DIR}/lib/kaldi/cudamatrix/kaldi-cudamatrix.a
    ${VOSK_DIR}/lib/kaldi/matrix/kaldi-matrix.a
    ${VOSK_DIR}/lib/kaldi/fstext/kaldi-fstext.a
    ${VOSK_DIR}/lib/kaldi/util/kaldi-util.a
    ${VOSK_DIR}/lib/kaldi/base/kaldi-base.a
    ${VOSK_DIR}/lib/openfst/libfst.a
    ${VOSK_DIR}/lib/openfst/libfstngram.a
    ${VOSK_DIR}/lib/openblas/libopenblas-r0.3.20.a
    ${VOSK_DIR}/lib/openblas/libblas.a
    ${VOSK_DIR}/lib/openblas/liblapack.a
    ${VOSK_DIR}/lib/openblas/libf2c.a
    -Wl,--end-group

    ${PORTAUDIO_DIR}/lib/libportaudio.so
)

target_compile_options(ufeel_detection_library PRIVATE
    ${TORCH_CXX_FLAGS}
)

add_library(ufeel_wrapper SHARED
    ${WRAPPER_DIR}/wrapper.cpp
)

set_target_properties(ufeel_wrapper PROPERTIES
    BUILD_RPATH
        "${OPENCV_DIR}/lib;${LIBTORCH_DIR}/lib;${PORTAUDIO_DIR}/lib"
    INSTALL_RPATH
        "\$ORIGIN/dependencies"
)

target_link_libraries(ufeel_wrapper
    ufeel_detection_library
)

if(CLANG_TIDY_COMMAND)
    set_target_properties(ufeel_wrapper PROPERTIES
        CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}"
    )
endif()

target_include_directories(ufeel_wrapper
    PUBLIC
        ${LIBRARY_DIR}/include
        ${WRAPPER_DIR}
    SYSTEM PUBLIC
        ${OPENCV_DIR}/include/opencv4
        ${TORCH_INCLUDE_DIRS}
)

install(
    TARGETS ufeel_wrapper
    LIBRARY DESTINATION .
)

install(
    FILES
        ${OPENCV_DIR}/lib/libopencv_core.so.4.8.1
        ${OPENCV_DIR}/lib/libopencv_imgproc.so.4.8.1
        ${OPENCV_DIR}/lib/libopencv_imgcodecs.so.4.8.1
        ${OPENCV_DIR}/lib/libopencv_dnn.so.4.8.1
        ${OPENCV_DIR}/lib/libopencv_objdetect.so.4.8.1
        ${OPENCV_DIR}/lib/libopencv_videoio.so.4.8.1
        ${OPENCV_DIR}/lib/libopencv_calib3d.so.4.8.1
        ${OPENCV_DIR}/lib/libopencv_features2d.so.4.8.1
        ${OPENCV_DIR}/lib/libopencv_flann.so.4.8.1
    DESTINATION dependencies
)

install(
    FILES
        ${LIBTORCH_DIR}/lib/libtorch.so
        ${LIBTORCH_DIR}/lib/libtorch_cpu.so
        ${LIBTORCH_DIR}/lib/libc10.so
        ${LIBTORCH_DIR}/lib/libgomp-98b21ff3.so.1
    DESTINATION dependencies
)

install(
    FILES
        ${PORTAUDIO_DIR}/lib/libportaudio.so.2.0.0
    DESTINATION dependencies
)

install(
    CODE [[
        file(GLOB OPENCV_LIBS
            "${CMAKE_INSTALL_PREFIX}/dependencies/libopencv_*.so.4.8.1"
        )

        foreach(LIB ${OPENCV_LIBS})
            get_filename_component(NAME "${LIB}" NAME)
            string(REGEX REPLACE "\\.so\\.4\\.8\\.1$" ".so.408" SONAME "${NAME}")
            execute_process(
                COMMAND "${CMAKE_COMMAND}" -E create_symlink
                    "${NAME}"
                    "${CMAKE_INSTALL_PREFIX}/dependencies/${SONAME}"
            )
        endforeach()

        execute_process(
            COMMAND "${CMAKE_COMMAND}" -E create_symlink
                "libportaudio.so.2.0.0"
                "${CMAKE_INSTALL_PREFIX}/dependencies/libportaudio.so.2"
        )
    ]]
)

install(
    DIRECTORY "${MODELS_DIR}/"
    DESTINATION models
)
# TODO: resolve the path for every single model accordingly (currently only vosk)
