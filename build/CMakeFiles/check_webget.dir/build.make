# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/nagomi/.local/share/Trash/files/sponge

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/nagomi/mymuduo/build

# Utility rule file for check_webget.

# Include the progress variables for this target.
include CMakeFiles/check_webget.dir/progress.make

CMakeFiles/check_webget:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/nagomi/mymuduo/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Testing webget..."
	/usr/bin/ctest --output-on-failure --timeout 10 -R 't_webget'

check_webget: CMakeFiles/check_webget
check_webget: CMakeFiles/check_webget.dir/build.make

.PHONY : check_webget

# Rule to build all files generated by this target.
CMakeFiles/check_webget.dir/build: check_webget

.PHONY : CMakeFiles/check_webget.dir/build

CMakeFiles/check_webget.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/check_webget.dir/cmake_clean.cmake
.PHONY : CMakeFiles/check_webget.dir/clean

CMakeFiles/check_webget.dir/depend:
	cd /home/nagomi/mymuduo/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/nagomi/.local/share/Trash/files/sponge /home/nagomi/.local/share/Trash/files/sponge /home/nagomi/mymuduo/build /home/nagomi/mymuduo/build /home/nagomi/mymuduo/build/CMakeFiles/check_webget.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/check_webget.dir/depend

