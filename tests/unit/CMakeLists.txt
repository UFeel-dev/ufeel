add_executable(
    run_unit_tests
    # Emotions/TestResNetImpl.cpp
    # Emotions/TestModel.cpp

    HeartRate/TestHeartRateDetector.cpp
)

target_link_libraries(
    run_unit_tests
    PRIVATE
        GTest::gtest_main
        ufeel_detection_library
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
