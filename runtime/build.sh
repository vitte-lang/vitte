#!/bin/bash
# Vitte Runtime Build Script

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="$SCRIPT_DIR/build"
INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"

echo "╔════════════════════════════════════════════════════╗"
echo "║       Vitte Runtime Build System                   ║"
echo "╚════════════════════════════════════════════════════╝"
echo

# Parse arguments
BUILD_TYPE="Release"
ENABLE_DEBUG=OFF
ENABLE_TESTS=ON
VERBOSE=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_TYPE="Debug"
            ENABLE_DEBUG=ON
            shift
            ;;
        --release)
            BUILD_TYPE="Release"
            shift
            ;;
        --no-tests)
            ENABLE_TESTS=OFF
            shift
            ;;
        --verbose)
            VERBOSE="--verbose"
            shift
            ;;
        --clean)
            echo "[*] Cleaning build directory..."
            rm -rf "$BUILD_DIR"
            echo "[✓] Clean complete"
            exit 0
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --debug        Build debug version with symbols"
            echo "  --release      Build optimized release (default)"
            echo "  --no-tests     Disable tests"
            echo "  --verbose      Show all build output"
            echo "  --clean        Clean build directory"
            echo "  --help         Show this help"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Create build directory
if [ ! -d "$BUILD_DIR" ]; then
    echo "[*] Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Run CMake
echo "[*] Configuring with CMake..."
cmake \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DENABLE_DEBUG="$ENABLE_DEBUG" \
    -DENABLE_TESTS="$ENABLE_TESTS" \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
    "$SCRIPT_DIR"

# Build
echo "[*] Building Vitte Runtime..."
cmake --build . $VERBOSE --config "$BUILD_TYPE"

# Run tests if enabled
if [ "$ENABLE_TESTS" = "ON" ]; then
    echo "[*] Running tests..."
    ctest --output-on-failure
fi

echo
echo "╔════════════════════════════════════════════════════╗"
echo "║              Build Complete!                       ║"
echo "╚════════════════════════════════════════════════════╝"
echo "Build type:  $BUILD_TYPE"
echo "Build dir:   $BUILD_DIR"
echo
echo "Next steps:"
echo "  ./build/vitte-runtime              - Run REPL"
echo "  ./build/vitte-runtime --test       - Run tests"
echo "  cmake --install .                  - Install system-wide"
echo
