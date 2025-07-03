#!/bin/bash
# clean.sh

# Remove the build directory
rm -rf build

# Remove CMake related files from the root directory
rm -f CMakeCache.txt
rm -rf CMakeFiles/

echo "Clean complete. Build directory and CMake files removed."