#!/bin/bash
# build.sh

# Create a build directory if it doesn't exist
mkdir -p build

# Navigate into the build directory
cd build

# Run CMake to configure the project
cmake ..

# Build the project
make

# Navigate back to the original directory
cd ..

echo "Build complete. Executables are in the 'build' directory."
