# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.20

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake

# The command to remove a file.
RM = /Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/nathaniel/Documents/Nathaniel/Programs/C++/Lightning

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/nathaniel/Documents/Nathaniel/Programs/C++/Lightning/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/Lightning_.dir/depend.make
# Include the progress variables for this target.
include CMakeFiles/Lightning_.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/Lightning_.dir/flags.make

# Object files for target Lightning_
Lightning__OBJECTS =

# External object files for target Lightning_
Lightning__EXTERNAL_OBJECTS =

lib/libLightning_.a: CMakeFiles/Lightning_.dir/build.make
lib/libLightning_.a: CMakeFiles/Lightning_.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/nathaniel/Documents/Nathaniel/Programs/C++/Lightning/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Linking CXX static library lib/libLightning_.a"
	$(CMAKE_COMMAND) -P CMakeFiles/Lightning_.dir/cmake_clean_target.cmake
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/Lightning_.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/Lightning_.dir/build: lib/libLightning_.a
.PHONY : CMakeFiles/Lightning_.dir/build

CMakeFiles/Lightning_.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/Lightning_.dir/cmake_clean.cmake
.PHONY : CMakeFiles/Lightning_.dir/clean

CMakeFiles/Lightning_.dir/depend:
	cd /Users/nathaniel/Documents/Nathaniel/Programs/C++/Lightning/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/nathaniel/Documents/Nathaniel/Programs/C++/Lightning /Users/nathaniel/Documents/Nathaniel/Programs/C++/Lightning /Users/nathaniel/Documents/Nathaniel/Programs/C++/Lightning/cmake-build-debug /Users/nathaniel/Documents/Nathaniel/Programs/C++/Lightning/cmake-build-debug /Users/nathaniel/Documents/Nathaniel/Programs/C++/Lightning/cmake-build-debug/CMakeFiles/Lightning_.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/Lightning_.dir/depend

