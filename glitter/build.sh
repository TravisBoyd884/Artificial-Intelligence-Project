#!/usr/bin/env bash
# Build the Glitter binary.
# Usage: ./build.sh [--clean]

set -e
cd "$(dirname "$0")"

if [[ "$1" == "--clean" ]]; then
    echo "Cleaning build directory..."
    rm -rf Build/CMakeFiles Build/Makefile Build/CMakeCache.txt Build/cmake_install.cmake
fi

cmake -S . -B Build -DCMAKE_BUILD_TYPE=Release 2>&1 | tail -5
cmake --build Build --parallel "$(nproc)"
echo "Binary: Build/Glitter/Glitter"
