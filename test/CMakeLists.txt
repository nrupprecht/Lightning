cmake_minimum_required(VERSION 3.14)

project(LightningTests LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(Lightning REQUIRED)
  enable_testing()
endif()

find_package(GTest REQUIRED)
include(GoogleTest)

# ---- Tests ----

file(GLOB_RECURSE PROJECT_TEST_CPPS source/**UT_*.cpp)

add_executable(Lightning_test ${PROJECT_TEST_CPPS})
target_link_libraries(
    Lightning_test PRIVATE
    Lightning::Lightning
    GTest::gtest
    GTest::gtest_main
)
target_compile_features(Lightning_test PRIVATE cxx_std_17)

gtest_discover_tests(Lightning_test)


# ---- End-of-file commands ----

add_folders(Test)