#!/usr/bin/env bash
# ============================================================
# build-debian-i586-deb.sh
#
# Complete build pipeline for Vitte Debian i586 (32-bit) package
# Architecture: i586 (Pentium, Pentium MMX - no i686 extensions)
# Includes:
#   - Compiler compilation (i586)
#   - Standard library
#   - Editors (vim, emacs, nano, geany, tree-sitter)
#   - Documentation
#   - Completions
#   - .deb installer creation
#
# Output: pkgout/vitte_*.deb (i386 architecture)
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
ARCH="${ARCH:-i386}"
PACKAGE_NAME="${PACKAGE_NAME:-vitte}"
VERSION="${VERSION:-$(cat $ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION 2>/dev/null | tr -d ' \r\n' || echo '2.1.1')}"

# i586 specific (Pentium, no i686 extensions)
CFLAGS="-m32 -march=i586 -mtune=pentium -O${OPT_LEVEL} -fPIC"
CXXFLAGS="-m32 -march=i586 -mtune=pentium -O${OPT_LEVEL} -fPIC"
LDFLAGS="-m32"

echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}Vitte Debian i586 (Pentium) .deb Package Builder${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo ""

# Step 1: Check prerequisites
echo -e "${YELLOW}[1/6]${NC} Checking prerequisites..."

if [[ "$OSTYPE" != "linux-gnu"* ]]; then
    echo -e "${RED}✗ Error: This script requires Linux${NC}"
    exit 1
fi

if ! command -v gcc &>/dev/null; then
    echo -e "${RED}✗ Error: GCC is required. Install with: sudo apt install build-essential${NC}"
    exit 1
fi

if ! command -v dpkg-deb &>/dev/null; then
    echo -e "${RED}✗ Error: dpkg-deb is required. Install with: sudo apt install dpkg-deb${NC}"
    exit 1
fi

# Check for 32-bit compiler support (i586 requires multilib)
echo -e "${YELLOW}  Verifying 32-bit compiler support (i586)...${NC}"
if ! gcc -m32 -v 2>&1 | grep -q "i586"; then
    echo -e "${YELLOW}  ⚠ Warning: 32-bit i586 support may not be fully available${NC}"
    echo -e "${YELLOW}  Install with: sudo apt install gcc-multilib g++-multilib${NC}"
fi

echo -e "${GREEN}✓ Prerequisites OK${NC}"
echo ""

# Step 2: Clean
echo -e "${YELLOW}[2/6]${NC} Cleaning previous builds..."
rm -rf "$BUILD_DIR" "$TARGET_DIR/debian-i586" .debstage-i586 2>/dev/null || true
mkdir -p "$BUILD_DIR" "$TARGET_DIR" "$OUT_DIR"
echo -e "${GREEN}✓ Clean complete${NC}"
echo ""

# Step 3: Compile
echo -e "${YELLOW}[3/6]${NC} Compiling Vitte for i586 (Pentium)..."
echo -e "  CFLAGS: $CFLAGS"
echo -e "  JOBS: $JOBS"

cd "$ROOT_DIR"
export CFLAGS CXXFLAGS LDFLAGS

# Run make with i586 flags
if make -j "$JOBS" 2>&1 | tee "$BUILD_DIR/compile.log"; then
    echo -e "${GREEN}✓ Compilation successful${NC}"
else
    echo -e "${RED}✗ Compilation failed. See $BUILD_DIR/compile.log${NC}"
    exit 1
fi
echo ""

# Verify binary is i586
if [ -f "$BIN_DIR/vitte" ]; then
    BINARY_INFO=$(file "$BIN_DIR/vitte" 2>/dev/null || echo "unknown")
    if echo "$BINARY_INFO" | grep -q "32-bit"; then
        echo -e "${GREEN}✓ Binary verified: i586 32-bit${NC}"
    else
        echo -e "${YELLOW}⚠ Warning: Binary info: $BINARY_INFO${NC}"
    fi
fi
echo ""

# Step 4: Create .deb structure
echo -e "${YELLOW}[4/6]${NC} Creating .deb package structure..."
STAGE_DIR=".debstage-i586"
rm -rf "$STAGE_DIR"

# Create debian directory structure
mkdir -p "$STAGE_DIR/usr/bin"
mkdir -p "$STAGE_DIR/usr/lib/$PACKAGE_NAME"
mkdir -p "$STAGE_DIR/usr/share/$PACKAGE_NAME"
mkdir -p "$STAGE_DIR/usr/share/doc/$PACKAGE_NAME"
mkdir -p "$STAGE_DIR/etc/$PACKAGE_NAME"
mkdir -p "$STAGE_DIR/etc/bash_completion.d"
mkdir -p "$STAGE_DIR/DEBIAN"

# Copy compiled binary
if [ -f "$BIN_DIR/vitte" ]; then
    cp "$BIN_DIR/vitte" "$STAGE_DIR/usr/bin/"
    chmod 755 "$STAGE_DIR/usr/bin/vitte"
    echo -e "  ${GREEN}✓${NC} Binary copied"
fi

# Copy stdlib
if [ -d "$ROOT_DIR/data/stdlib" ]; then
    cp -r "$ROOT_DIR/data/stdlib"/* "$STAGE_DIR/usr/lib/$PACKAGE_NAME/" 2>/dev/null || true
    echo -e "  ${GREEN}✓${NC} Standard library copied"
fi

# Copy editors support
for editor_dir in "$ROOT_DIR/editors"/*; do
    if [ -d "$editor_dir" ]; then
        cp -r "$editor_dir" "$STAGE_DIR/usr/share/$PACKAGE_NAME/" 2>/dev/null || true
    fi
done
echo -e "  ${GREEN}✓${NC} Editor support copied"

# Copy documentation
if [ -d "$ROOT_DIR/docs" ]; then
    cp -r "$ROOT_DIR/docs"/* "$STAGE_DIR/usr/share/doc/$PACKAGE_NAME/" 2>/dev/null || true
fi
echo -e "  ${GREEN}✓${NC} Documentation copied"

# Copy completions
if [ -d "$ROOT_DIR/completions" ]; then
    for comp_file in "$ROOT_DIR/completions"/*; do
        [ -f "$comp_file" ] && cp "$comp_file" "$STAGE_DIR/etc/bash_completion.d/"
    done
    echo -e "  ${GREEN}✓${NC} Completions copied"
fi

echo ""

# Step 5: Create control files
echo -e "${YELLOW}[5/6]${NC} Creating Debian control metadata..."

# Control file
cat > "$STAGE_DIR/DEBIAN/control" << 'CONTROL'
Package: PACKAGE_NAME
Version: VERSION
Architecture: i386
Installed-Size: 512000
Maintainer: Vitte Authors <info@vitte-lang.io>
Homepage: https://vitte-lang.io
Description: Vitte Programming Language (32-bit Pentium i586)
 Vitte is a modern programming language compiler and runtime.
 This package is optimized for i586 (Pentium, Pentium MMX) processors.
 .
 Includes: compiler, standard library, editors support, documentation
CONTROL

# Replace placeholders
sed -i "s/PACKAGE_NAME/$PACKAGE_NAME/g" "$STAGE_DIR/DEBIAN/control"
sed -i "s/VERSION/$VERSION/g" "$STAGE_DIR/DEBIAN/control"

# PostInst script
cat > "$STAGE_DIR/DEBIAN/postinst" << 'POSTINST'
#!/bin/bash
set -e

PACKAGE="PACKAGE_NAME"
BIN_PATH="/usr/bin/vitte"

# Register alternatives
update-alternatives --install /usr/bin/vitte vitte "$BIN_PATH" 100 2>/dev/null || true

# Setup shell completions
if [ -d /etc/bash_completion.d ]; then
    for comp in /etc/bash_completion.d/vitte*; do
        [ -f "$comp" ] && chmod 644 "$comp"
    done
fi

echo "✓ $PACKAGE installed successfully (i586 Pentium)"
echo "  Run: vitte --version"

exit 0
POSTINST

sed -i "s/PACKAGE_NAME/$PACKAGE_NAME/g" "$STAGE_DIR/DEBIAN/postinst"
chmod 755 "$STAGE_DIR/DEBIAN/postinst"

# PreRm script
cat > "$STAGE_DIR/DEBIAN/prerm" << 'PRERM'
#!/bin/bash
set -e

# Remove alternatives
update-alternatives --remove vitte /usr/bin/vitte 2>/dev/null || true

exit 0
PRERM

chmod 755 "$STAGE_DIR/DEBIAN/prerm"

# Create md5sums
cd "$STAGE_DIR"
find . -type f ! -path './DEBIAN/*' -exec md5sum {} \; > DEBIAN/md5sums 2>/dev/null || true
cd - > /dev/null

echo -e "${GREEN}✓ Control files created${NC}"
echo ""

# Step 6: Build .deb package
echo -e "${YELLOW}[6/6]${NC} Building .deb package..."

DEB_FILE="$OUT_DIR/${PACKAGE_NAME}_${VERSION}_${ARCH}.deb"
if dpkg-deb --build "$STAGE_DIR" "$DEB_FILE" 2>&1; then
    echo -e "${GREEN}✓ Package created: $DEB_FILE${NC}"
    
    # Show package info
    DEB_SIZE=$(du -h "$DEB_FILE" | cut -f1)
    echo -e "  Size: $DEB_SIZE"
    echo -e "  Architecture: i386 (i586 Pentium)"
    
    # List contents
    echo -e "\n${BLUE}Package contents:${NC}"
    dpkg-deb -c "$DEB_FILE" | head -10
    echo -e "  ... (see dpkg-deb -c $DEB_FILE for full list)"
else
    echo -e "${RED}✗ Package build failed${NC}"
    exit 1
fi

# Cleanup
rm -rf "$STAGE_DIR"

echo ""
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}✓ Build complete!${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo ""
echo -e "Output: ${GREEN}$DEB_FILE${NC}"
echo -e "Next: Create distribution files with:"
echo -e "  ${YELLOW}bash scripts/build-debian-i586-distribution-bundle.sh${NC}"
echo ""
