# CMakeLists.txt for testing

enable_testing()
find_package(GTest REQUIRED)

# Find all test files.
file(GLOB_RECURSE PROJECT_TEST_CPPS UT_*.cpp)

# Add the complete test suite.
add_executable              (Lightning_test setup/main.cpp ${PROJECT_TEST_CPPS})
target_compile_features     (Lightning_test PRIVATE cxx_std_17)
target_link_libraries       (Lightning_test PRIVATE GTest::gtest Lightning::Lightning)  # GTest::gtest_main

# Add each test file as an individual test
set                         (TEST_MAIN_SOURCES setup/main.cpp)
file                        (GLOB_RECURSE PROJECT_TEST_CPPS UT_*.cpp)
foreach(_SRC ${PROJECT_TEST_CPPS})
    get_filename_component  (_NAME ${_SRC} NAME_WE)
    add_executable          (${_NAME} ${_SRC} ${TEST_MAIN_SOURCES})
    target_link_libraries   (${_NAME} PUBLIC Lightning::Lightning GTest::gtest GTest::gmock)
    target_compile_features (${_NAME} PRIVATE cxx_std_17)
    add_test                (NAME ${_NAME} COMMAND ${_NAME})
endforeach ()
