add_executable(
    run_wrapper_unit_tests
    src/internal/TestErrorHandling.cpp
    src/internal/TestFrameConversion.cpp
)

target_include_directories(run_wrapper_unit_tests PRIVATE
    "${WRAPPER_DIR}/include"
    "${WRAPPER_DIR}/src"
    "${OPENCV_DIR}/include/opencv4"
)

target_link_libraries(
    run_wrapper_unit_tests
    PRIVATE
        GTest::gtest_main
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
