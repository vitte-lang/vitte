#!/usr/bin/env bash
# ============================================================
# build-windows-x86_64-exe.sh
#
# Build Vitte for Windows x86_64 (64-bit) using MinGW-w64
# Can be run on Linux or Windows
#
# Usage: bash scripts/build-windows-x86_64-exe.sh
# Or on Windows: scripts\build-windows-x86_64-exe.bat
#
# Output: pkgout/vitte-2.1.1-x86_64-installer.exe
# ============================================================

set -euo pipefail

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Configuration
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/build}"
BIN_DIR="${BIN_DIR:-$ROOT_DIR/bin}"
TARGET_DIR="${TARGET_DIR:-$ROOT_DIR/target}"
OUT_DIR="${OUT_DIR:-$ROOT_DIR/pkgout}"
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 4)}"
OPT_LEVEL="${OPT_LEVEL:-2}"
VERSION="${VERSION:-$(cat $ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION 2>/dev/null | tr -d ' \r\n' || echo '2.1.1')}"

# Windows x86_64 compilation flags
CFLAGS="-m64 -O${OPT_LEVEL} -fPIC"
CXXFLAGS="-m64 -O${OPT_LEVEL} -fPIC"
LDFLAGS="-m64"

echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}Vitte Windows x86_64 (.exe) Installer Builder${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo ""

# Step 1: Check prerequisites
echo -e "${YELLOW}[1/6]${NC} Checking prerequisites..."

# Check for MinGW-w64 compiler
if command -v x86_64-w64-mingw32-gcc &>/dev/null; then
    echo -e "${GREEN}✓ MinGW-w64 compiler found${NC}"
    CC="x86_64-w64-mingw32-gcc"
    CXX="x86_64-w64-mingw32-g++"
elif command -v gcc &>/dev/null; then
    echo -e "${YELLOW}⚠ MinGW-w64 not found, using native GCC${NC}"
    echo -e "${YELLOW}  Install: sudo apt install mingw-w64${NC}"
    CC="gcc"
    CXX="g++"
else
    echo -e "${RED}✗ No C compiler found${NC}"
    exit 1
fi

# Check for NSIS
if ! command -v makensis &>/dev/null; then
    echo -e "${RED}✗ NSIS not found (makensis)${NC}"
    echo -e "${YELLOW}  Install: sudo apt install nsis${NC}"
    exit 1
fi
echo -e "${GREEN}✓ NSIS installer builder found${NC}"

echo -e "${GREEN}✓ Prerequisites OK${NC}"
echo ""

# Step 2: Clean
echo -e "${YELLOW}[2/6]${NC} Cleaning previous builds..."
rm -rf "$BUILD_DIR" "$TARGET_DIR/windows-x86_64" .stage-windows-x86_64 2>/dev/null || true
mkdir -p "$BUILD_DIR" "$TARGET_DIR" "$OUT_DIR"
echo -e "${GREEN}✓ Clean complete${NC}"
echo ""

# Step 3: Compile
echo -e "${YELLOW}[3/6]${NC} Compiling Vitte for Windows x86_64..."
echo -e "  CFLAGS: $CFLAGS"
echo -e "  JOBS: $JOBS"

cd "$ROOT_DIR"
export CC CXX CFLAGS CXXFLAGS LDFLAGS

# For Windows, we typically build on Windows or use cross-compilation
# This is a template - actual compilation depends on build system
if make -j "$JOBS" 2>&1 | tee "$BUILD_DIR/compile.log"; then
    echo -e "${GREEN}✓ Compilation successful${NC}"
else
    echo -e "${RED}✗ Compilation failed. See $BUILD_DIR/compile.log${NC}"
    exit 1
fi
echo ""

# Step 4: Prepare staging directory
echo -e "${YELLOW}[4/6]${NC} Preparing Windows installation files..."
STAGE_DIR=".stage-windows-x86_64"
rm -rf "$STAGE_DIR"

# Create Windows directory structure
mkdir -p "$STAGE_DIR/bin"
mkdir -p "$STAGE_DIR/lib"
mkdir -p "$STAGE_DIR/data/stdlib"
mkdir -p "$STAGE_DIR/data/lib"
mkdir -p "$STAGE_DIR/editors"
mkdir -p "$STAGE_DIR/examples"

# Copy compiled binary (with .exe extension for Windows)
if [ -f "$BIN_DIR/vitte" ]; then
    cp "$BIN_DIR/vitte" "$STAGE_DIR/bin/vitte-x86_64.exe"
    chmod +x "$STAGE_DIR/bin/vitte-x86_64.exe"
    echo -e "  ${GREEN}✓${NC} Binary copied and renamed to .exe"
fi

# Copy stdlib
if [ -d "$ROOT_DIR/data/stdlib" ]; then
    cp -r "$ROOT_DIR/data/stdlib"/* "$STAGE_DIR/data/stdlib/" 2>/dev/null || true
    echo -e "  ${GREEN}✓${NC} Standard library copied"
fi

# Copy editors support
if [ -d "$ROOT_DIR/editors" ]; then
    cp -r "$ROOT_DIR/editors"/* "$STAGE_DIR/editors/" 2>/dev/null || true
    echo -e "  ${GREEN}✓${NC} Editor support copied"
fi

# Copy documentation
if [ -d "$ROOT_DIR/docs" ]; then
    cp -r "$ROOT_DIR/docs"/* "$STAGE_DIR/" 2>/dev/null || true
    cp "$ROOT_DIR/README.md" "$STAGE_DIR/" 2>/dev/null || true
    cp "$ROOT_DIR/LICENSE" "$STAGE_DIR/" 2>/dev/null || true
    echo -e "  ${GREEN}✓${NC} Documentation copied"
fi

# Copy examples
if [ -d "$ROOT_DIR/examples" ]; then
    cp -r "$ROOT_DIR/examples"/* "$STAGE_DIR/examples/" 2>/dev/null || true
    echo -e "  ${GREEN}✓${NC} Examples copied"
fi

echo ""

# Step 5: Generate NSIS installer
echo -e "${YELLOW}[5/6]${NC} Generating NSIS installer script..."

NSIS_SCRIPT="$STAGE_DIR/vitte-installer.nsi"
cp "toolchain/scripts/package/windows/vitte-installer.nsi" "$NSIS_SCRIPT"

# Update version in NSIS script
sed -i "s/!define VERSION \"[^\"]*\"/!define VERSION \"$VERSION\"/g" "$NSIS_SCRIPT"

echo -e "${GREEN}✓ NSIS script prepared${NC}"
echo ""

# Step 6: Build installer
echo -e "${YELLOW}[6/6]${NC} Building Windows x86_64 installer..."

cd "$STAGE_DIR"
EXE_FILE="$OUT_DIR/vitte-${VERSION}-x86_64-installer.exe"

if makensis -DOUTDIR="$OUT_DIR" "$NSIS_SCRIPT" 2>&1; then
    echo -e "${GREEN}✓ Installer created${NC}"
    
    if [ -f "$EXE_FILE" ]; then
        EXE_SIZE=$(du -h "$EXE_FILE" | cut -f1)
        echo -e "  File: $EXE_FILE"
        echo -e "  Size: $EXE_SIZE"
        echo -e "  Architecture: x86_64 (64-bit)"
    fi
else
    echo -e "${RED}✗ Installer build failed${NC}"
    exit 1
fi

cd "$ROOT_DIR"

# Cleanup
rm -rf "$STAGE_DIR"

echo ""
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}✓ Build complete!${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo ""
echo -e "Output: ${GREEN}$EXE_FILE${NC}"
echo -e "Next: Create checksums and documentation with:"
echo -e "  ${YELLOW}bash scripts/build-windows-distribution-bundle.sh${NC}"
echo ""
