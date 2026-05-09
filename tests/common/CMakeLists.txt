add_executable(common_tests
    ChecksumTest.cpp
    BufferTest.cpp
)
target_link_libraries(common_tests PRIVATE common GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(common_tests)
