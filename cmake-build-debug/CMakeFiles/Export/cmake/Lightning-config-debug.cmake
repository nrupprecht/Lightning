#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Lightning" for configuration "Debug"
set_property(TARGET Lightning APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(Lightning PROPERTIES
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libLightning.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS Lightning )
list(APPEND _IMPORT_CHECK_FILES_FOR_Lightning "${_IMPORT_PREFIX}/lib/libLightning.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
