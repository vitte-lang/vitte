#!/bin/bash
# build.sh - Simple build script for vitte-bench

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
CMAKE_FLAGS="${CMAKE_FLAGS:--DCMAKE_BUILD_TYPE=Release}"

echo "[vitte-bench] Building benchmarks..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake $CMAKE_FLAGS "$SCRIPT_DIR"
cmake --build . -j "$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"

echo "[vitte-bench] Build complete: $BUILD_DIR/benchc"
