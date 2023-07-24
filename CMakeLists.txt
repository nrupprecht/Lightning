cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    Lightning
    VERSION 0.1.0
    DESCRIPTION "Lightning logging library"
    HOMEPAGE_URL "https://github.com/nrupprecht/Lightning"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(Lightning_Lightning INTERFACE)
add_library(Lightning::Lightning ALIAS Lightning_Lightning)

set_property(
    TARGET Lightning_Lightning PROPERTY
    EXPORT_NAME Lightning
)

target_include_directories(
    Lightning_Lightning ${warning_guard}
    INTERFACE
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_compile_features(Lightning_Lightning INTERFACE cxx_std_17)

add_subdirectory("${PROJECT_SOURCE_DIR}/applications")

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT Lightning_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of Lightning"
  )
endif()

include(cmake/dev-mode.cmake)
