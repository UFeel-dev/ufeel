add_executable(
    run_unit_tests
    test.cpp
)

target_link_libraries(
    run_unit_tests
    PRIVATE
        GTest::gtest_main
        
)

include(GoogleTest)

gtest_discover_tests(
    run_unit_tests
)
