cmake_minimum_required(VERSION 3.20)
project(ufeel_detection_library)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ======================
# clang-tidy
# ======================
option(CLANG_TIDY_COLOR "Force colored clang-tidy diagnostics" ON)
option(ENABLE_CLANG_TIDY "Run clang-tidy as part of the compilation" ON)
option(CLANG_TIDY_AS_ERRORS "Honour the WarningsAsErrors list in .clang-tidy" ON)

if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)

    if(CLANG_TIDY_EXE)
        # The regex has to stay quoted: unquoted parens/pipes are CMake syntax.
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

# ======================
# Paths
# ======================
set(THIRDPARTY_DIR ${CMAKE_SOURCE_DIR}/3rdparty)
set(LIBRARY_DIR ${CMAKE_SOURCE_DIR}/Library)
set(WRAPPER_DIR ${CMAKE_SOURCE_DIR}/Wrapper)
set(CSHARP_DIR ${CMAKE_SOURCE_DIR}/csharp)
set(CSHARP_DEBUG ${CSHARP_DIR}/bin/Debug/net8.0)
set(CSHARP_RELEASE ${CSHARP_DIR}/bin/Release/net8.0)

set(OPENCV_DIR ${THIRDPARTY_DIR}/opencv)
set(LIBTORCH_DIR ${THIRDPARTY_DIR}/libtorch)
set(TFLITE_DIR ${THIRDPARTY_DIR}/tensorflowlite)
set(VOSK_DIR ${THIRDPARTY_DIR}/vosk)


# ======================
# LibTorch
# ======================
set(Torch_DIR ${LIBTORCH_DIR}/share/cmake/Torch)
find_package(Torch REQUIRED)

# ======================
# Library
# ======================
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

    ${LIBRARY_DIR}/src/SpeechToText/SpeechToTextDetector.cpp
)

if(CLANG_TIDY_COMMAND)
    set_target_properties(ufeel_detection_library PROPERTIES
        CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}"
    )
endif()

# ======================
# Includes
# ======================
target_include_directories(ufeel_detection_library 
    PUBLIC
        ${LIBRARY_DIR}/include
        
    SYSTEM PUBLIC
        ${OPENCV_DIR}/include
        ${TORCH_INCLUDE_DIRS}
        ${TFLITE_DIR}/include
        ${VOSK_DIR}/include
)

# ======================
# Link libs
# ======================
target_link_libraries(ufeel_detection_library PUBLIC
    ${OPENCV_DIR}/lib/libopencv_world.so
    ${TORCH_LIBRARIES}

    ${TFLITE_DIR}/lib/libtensorflow-lite.a
    ${TFLITE_DIR}/lib/flatbuffers/libflatbuffers.a # Need to be compiled after TensorFlow
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

    portaudio
)

# ======================
# Torch flags
# ======================
target_compile_options(ufeel_detection_library PRIVATE
    ${TORCH_CXX_FLAGS}
)

# ======================
# Wrapper (C API)
# ======================
add_library(ufeel_wrapper SHARED
    ${WRAPPER_DIR}/wrapper.cpp
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
    
    SYSTEM PUBLIC
        ${OPENCV_DIR}/include
        ${TORCH_INCLUDE_DIRS}
)

# ======================
# C# Test
# ======================
add_custom_command(TARGET ufeel_wrapper POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CSHARP_DEBUG}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CSHARP_RELEASE}

    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:ufeel_wrapper>
        ${CSHARP_DEBUG}

    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:ufeel_wrapper>
        ${CSHARP_RELEASE}
)
