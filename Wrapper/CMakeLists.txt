add_library(ufeel_wrapper SHARED
    src/processor.cpp
    src/emotions.cpp
    src/eye_tracking.cpp
    src/speech_to_text.cpp
    src/heart_rate.cpp
)

target_compile_definitions(ufeel_wrapper
    PRIVATE
        UFEEL_BUILDING_LIBRARY
)

target_include_directories(ufeel_wrapper
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/include"
)

target_link_libraries(ufeel_wrapper
    PRIVATE
        ufeel_detection_library
)

target_compile_options(ufeel_wrapper
    PRIVATE
        ${TORCH_CXX_FLAGS}
)

target_link_options(ufeel_wrapper
    PRIVATE
        "-Wl,--no-undefined"
)

set_target_properties(ufeel_wrapper PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    BUILD_RPATH
        "${OPENCV_DIR}/lib;${LIBTORCH_DIR}/lib;${PORTAUDIO_DIR}/lib"
    INSTALL_RPATH
        "$ORIGIN/dependencies"
    INSTALL_RPATH_USE_LINK_PATH FALSE
)

if(UNIX AND NOT APPLE)
    target_link_options(ufeel_wrapper
        PRIVATE
            "-Wl,--exclude-libs,ALL"
            "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/ufeel.map"
    )
endif()

install(
    TARGETS ufeel_wrapper
    LIBRARY DESTINATION .
    RUNTIME DESTINATION .
)

install(
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/include/ufeel.h"
    DESTINATION include
)
