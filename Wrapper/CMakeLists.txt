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

if(UFEEL_ENABLE_EYE_TRACKING)
    target_link_libraries(ufeel_wrapper
        PRIVATE
            "${EYE_TRACKING_LINK_LIBRARIES}"
    )
endif()

target_compile_options(ufeel_wrapper
    PRIVATE
        ${TORCH_CXX_FLAGS}
)

set_target_properties(ufeel_wrapper PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    BUILD_RPATH
        "${OPENCV_DIR}/lib;${LIBTORCH_DIR}/lib;${PORTAUDIO_DIR}/lib;${VOSK_DIR}/lib"
    INSTALL_RPATH_USE_LINK_PATH FALSE
)

if(APPLE)
    set_target_properties(ufeel_wrapper PROPERTIES
        INSTALL_RPATH "@loader_path/dependencies"
        LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ufeel.exp"
    )
    target_link_options(ufeel_wrapper
        PRIVATE
            "-Wl,-exported_symbols_list,${CMAKE_CURRENT_SOURCE_DIR}/ufeel.exp"
    )
elseif(UNIX)
    set_target_properties(ufeel_wrapper PROPERTIES
        INSTALL_RPATH "$ORIGIN/dependencies"
    )
    target_link_options(ufeel_wrapper
        PRIVATE
            "-Wl,--no-undefined"
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
