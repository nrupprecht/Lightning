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

# Find tests even in nexted directories.
file(GLOB_RECURSE PROJECT_TEST_CPPS source/**UT_*.cpp)

message("Found tests: ${PROJECT_TEST_CPPS}")

foreach(_SOURCE ${PROJECT_TEST_CPPS})
  get_filename_component(_NAME ${_SOURCE} NAME_WE)
  add_executable        (${_NAME} ${_SOURCE})
  target_link_libraries (${_NAME} ${PROJECT_NAME})
  # NOTE: I used to just have add_test(${_NAME} ${_NAME}) here, but then when I would try to run all CTests,
  #       it couldn't find the executables.
  add_test              (NAME ${_NAME} COMMAND ${_NAME})
  #set_property          (TARGET ${_NAME} PROPERTY FOLDER tests)
  #assign_source_group   (${_SOURCE})
endforeach()


# ---- End-of-file commands ----

add_folders(Test)
