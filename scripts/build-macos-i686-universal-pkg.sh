#!/usr/bin/env bash
# ============================================================
# build-macos-i686-universal-pkg.sh
#
# Complete build pipeline for Vitte macOS i686 package
# Architecture: i686 (32-bit, Intel, iMac 2006+)
# Includes:
#   - Compiler compilation (32-bit i686)
#   - Standard library
#   - Editors (vim, emacs, nano)
#   - Documentation
#   - Completions
#   - .pkg installer creation (universal binary)
#
# Output: pkgout/vitte_*.pkg (i686 architecture)
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
JOBS="${JOBS:-$(sysctl -n hw.ncpu 2>/dev/null || echo 4)}"
OPT_LEVEL="${OPT_LEVEL:-2}"
BUILD_TARGET="${BUILD_TARGET:-bootstrap-all}"
ARCH="${ARCH:-i686}"
PACKAGE_NAME="${PACKAGE_NAME:-vitte}"
VERSION="${VERSION:-$(cat $ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION 2>/dev/null | tr -d ' \r\n' || echo '2.1.1')}"

# i686 specific (32-bit Intel, iMac 2006+)
CFLAGS="-m32 -march=i686 -mtune=generic -O${OPT_LEVEL} -fPIC"
CXXFLAGS="-m32 -march=i686 -mtune=generic -O${OPT_LEVEL} -fPIC"
LDFLAGS="-m32"

echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}Vitte macOS i686 (32-bit, iMac 2006+) .pkg Package Builder${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo ""

# Step 1: Check prerequisites
echo -e "${YELLOW}[1/6]${NC} Checking prerequisites..."

if [[ "$OSTYPE" != "darwin"* ]]; then
    echo -e "${RED}✗ Error: This script requires macOS${NC}"
    exit 1
fi

if ! command -v pkgbuild &>/dev/null; then
    echo -e "${RED}✗ Error: pkgbuild is required (Xcode Command Line Tools)${NC}"
    echo -e "${YELLOW}  Install: xcode-select --install${NC}"
    exit 1
fi

if ! command -v clang &>/dev/null; then
    echo -e "${RED}✗ Error: Clang compiler not found${NC}"
    exit 1
fi

# Check for 32-bit support
echo -e "${YELLOW}  Verifying 32-bit support (i686)...${NC}"
if ! clang -m32 -v 2>&1 | grep -q "i686"; then
    echo -e "${YELLOW}  ⚠ Warning: 32-bit i686 support may be limited${NC}"
    echo -e "${YELLOW}  Note: Requires macOS 10.4-10.7.5 for full i686 support${NC}"
fi

echo -e "${GREEN}✓ Prerequisites OK${NC}"
echo ""

# Step 2: Clean
echo -e "${YELLOW}[2/6]${NC} Cleaning previous builds..."
rm -rf "$TARGET_DIR/macos-i686" .pkgstage-i686 2>/dev/null || true
mkdir -p "$BUILD_DIR" "$TARGET_DIR" "$OUT_DIR"
rm -f "$OUT_DIR/${PACKAGE_NAME}-${VERSION}-i686.pkg"
echo -e "${GREEN}✓ Clean complete${NC}"
echo ""

# Step 3: Compile
echo -e "${YELLOW}[3/6]${NC} Compiling Vitte for macOS i686 (32-bit)..."
echo -e "  CFLAGS: $CFLAGS"
echo -e "  JOBS: $JOBS"

cd "$ROOT_DIR"
export CFLAGS CXXFLAGS LDFLAGS

if make -j "$JOBS" "$BUILD_TARGET" 2>&1 | tee "$BUILD_DIR/compile.log"; then
    echo -e "${GREEN}✓ Compilation successful${NC}"
else
    echo -e "${RED}✗ Compilation failed. See $BUILD_DIR/compile.log${NC}"
    exit 1
fi
echo ""

# Verify binary is i686
if [ -f "$BIN_DIR/vitte" ]; then
    BINARY_INFO=$(file "$BIN_DIR/vitte" 2>/dev/null || echo "unknown")
    if echo "$BINARY_INFO" | grep -q "i386"; then
        echo -e "${GREEN}✓ Binary verified: i686 32-bit${NC}"
    else
        echo -e "${YELLOW}⚠ Warning: Binary info: $BINARY_INFO${NC}"
    fi
fi
echo ""

# Step 4: Create .pkg structure
echo -e "${YELLOW}[4/6]${NC} Creating .pkg package structure..."
STAGE_DIR=".pkgstage-i686"
INSTALL_DIR="$STAGE_DIR/Library/Vitte"

rm -rf "$STAGE_DIR"
mkdir -p "$INSTALL_DIR/bin"
mkdir -p "$INSTALL_DIR/lib"
mkdir -p "$INSTALL_DIR/share"
mkdir -p "$INSTALL_DIR/share/src/vitte/packages"
mkdir -p "$INSTALL_DIR/share/src/vitte/stdlib"
mkdir -p "$INSTALL_DIR/share/src/vitte/compiler"
mkdir -p "$STAGE_DIR/usr/local/bin"

# Copy compiled binary
if [ -f "$BIN_DIR/vitte" ]; then
    cp "$BIN_DIR/vitte" "$INSTALL_DIR/bin/"
    chmod 755 "$INSTALL_DIR/bin/vitte"
    # Create symlink in /usr/local/bin
    ln -sf "/Library/Vitte/bin/vitte" "$STAGE_DIR/usr/local/bin/vitte"
    echo -e "  ${GREEN}✓${NC} Binary copied"
fi

# Copy source packages, stdlib and compiler sources
if [ -d "$ROOT_DIR/src/vitte/packages" ]; then
    cp -r "$ROOT_DIR/src/vitte/packages"/* "$INSTALL_DIR/share/src/vitte/packages/" 2>/dev/null || true
fi
if [ -d "$ROOT_DIR/src/vitte/stdlib" ]; then
    cp -r "$ROOT_DIR/src/vitte/stdlib"/* "$INSTALL_DIR/share/src/vitte/stdlib/" 2>/dev/null || true
fi
if [ -d "$ROOT_DIR/src/vitte/compiler" ]; then
    cp -r "$ROOT_DIR/src/vitte/compiler"/* "$INSTALL_DIR/share/src/vitte/compiler/" 2>/dev/null || true
fi
echo -e "  ${GREEN}✓${NC} Source packages, standard library and compiler sources copied"

# Copy editors support
for editor_dir in "$ROOT_DIR/editors"/*; do
    if [ -d "$editor_dir" ]; then
        cp -r "$editor_dir" "$INSTALL_DIR/share/" 2>/dev/null || true
    fi
done
echo -e "  ${GREEN}✓${NC} Editor support copied"

# Copy documentation
if [ -d "$ROOT_DIR/docs" ]; then
    mkdir -p "$INSTALL_DIR/share/doc"
    cp -r "$ROOT_DIR/docs"/* "$INSTALL_DIR/share/doc/" 2>/dev/null || true
fi
echo -e "  ${GREEN}✓${NC} Documentation copied"

# Copy completions
if [ -d "$ROOT_DIR/completions" ]; then
    mkdir -p "$INSTALL_DIR/share/completions"
    cp "$ROOT_DIR/completions"/* "$INSTALL_DIR/share/completions/" 2>/dev/null || true
    echo -e "  ${GREEN}✓${NC} Completions copied"
fi

echo ""

# Step 5: Create postinstall script
echo -e "${YELLOW}[5/6]${NC} Creating installation scripts..."

SCRIPTS_DIR="$STAGE_DIR/scripts"
mkdir -p "$SCRIPTS_DIR"

cat > "$SCRIPTS_DIR/postinstall" << 'POSTINSTALL'
#!/bin/bash
set -e

# Update PATH in shell profiles
for profile in ~/.bashrc ~/.bash_profile ~/.zshrc ~/.profile; do
    if [ -f "$profile" ]; then
        if ! grep -q "/usr/local/bin/vitte" "$profile"; then
            echo 'export PATH="/usr/local/bin:$PATH"' >> "$profile"
        fi
    fi
done

# Create user library directory
mkdir -p ~/Library/Vitte/
chmod 755 ~/Library/Vitte/

# Register shell completions
if [ -d ~/Library/Vitte/share/completions ]; then
    for comp in ~/Library/Vitte/share/completions/*; do
        chmod 644 "$comp"
    done
fi

echo "✓ Vitte installed successfully (i686 32-bit)"
exit 0
POSTINSTALL

chmod +x "$SCRIPTS_DIR/postinstall"
echo -e "${GREEN}✓ Installation scripts created${NC}"

echo ""

# Step 6: Build .pkg package
echo -e "${YELLOW}[6/6]${NC} Building macOS .pkg package..."

PKG_FILE="$OUT_DIR/${PACKAGE_NAME}-${VERSION}-i686.pkg"

if pkgbuild --root "$STAGE_DIR" \
            --scripts "$SCRIPTS_DIR" \
            --identifier "io.vitte-lang.vitte.i686" \
            --version "$VERSION" \
            --install-location "/" \
            "$PKG_FILE" 2>&1; then
    echo -e "${GREEN}✓ Package created: $PKG_FILE${NC}"
    
    # Show package info
    PKG_SIZE=$(du -h "$PKG_FILE" | cut -f1)
    echo -e "  Size: $PKG_SIZE"
    echo -e "  Architecture: i686 (32-bit, iMac 2006+)"
    
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
echo -e "Output: ${GREEN}$PKG_FILE${NC}"
echo -e "Next: Create distribution files with:"
echo -e "  ${YELLOW}bash scripts/build-distribution-bundle-i686.sh${NC}"
echo ""
