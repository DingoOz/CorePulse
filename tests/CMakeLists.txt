find_package(GTest CONFIG REQUIRED)

# Test executable
add_executable(corepulse_tests
    test_main.cpp
)

target_link_libraries(corepulse_tests
    PRIVATE
        GTest::gtest
        GTest::gtest_main
)

# Add tests to CTest
add_test(NAME corepulse_tests COMMAND corepulse_tests)