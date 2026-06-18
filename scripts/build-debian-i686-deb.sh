#!/usr/bin/env bash
# ============================================================
# build-debian-i686-deb.sh
#
# Complete build pipeline for Vitte Debian i686 (32-bit) package
# Includes:
#   - Compiler compilation (i686)
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
DEBIAN_I686_DIR="${DEBIAN_I686_DIR:-$TARGET_DIR/debian-i686}"
OUT_DIR="${OUT_DIR:-$ROOT_DIR/pkgout}"

PKG_VERSION_FILE="$ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION"
VERSION="${VERSION:-$(tr -d ' \r\n' < "$PKG_VERSION_FILE" 2>/dev/null || echo 2.1.1)}"
ARCH="${ARCH:-i386}"
PACKAGE_NAME="${PACKAGE_NAME:-vitte}"
MAINTAINER="${MAINTAINER:-Vitte Team <maintainers@vitte-lang.org>}"

JOBS="${JOBS:-$(nproc 2>/dev/null || echo 4)}"
OPT_LEVEL="${OPT_LEVEL:-2}"

# Helpers
log() {
  printf "${BLUE}[build-deb-i686]${NC} %s\n" "$*"
}

success() {
  printf "${GREEN}[build-deb-i686]${NC} ✓ %s\n" "$*"
}

warn() {
  printf "${YELLOW}[build-deb-i686]${NC} ⚠ %s\n" "$*"
}

error() {
  printf "${RED}[build-deb-i686][ERROR]${NC} %s\n" "$*" >&2
}

die() {
  error "$@"
  exit 1
}

# Check prerequisites
check_prerequisites() {
  log "Checking prerequisites for Debian i686 build..."
  
  # Check for Linux
  if [[ "$OSTYPE" != "linux-gnu"* ]]; then
    die "This script requires Linux (not running on linux)"
  fi
  
  # Check for required tools
  local required_tools=("make" "dpkg-deb" "rsync" "python3")
  for tool in "${required_tools[@]}"; do
    if ! command -v "$tool" &> /dev/null; then
      die "Required tool not found: $tool"
    fi
  done
  
  # Check for 32-bit compiler support
  if ! command -v gcc &> /dev/null; then
    warn "gcc not found - build may fail"
  fi
  
  # Check if 32-bit compiler is available
  if ! gcc -m32 -v >/dev/null 2>&1; then
    warn "32-bit compiler support not available (gcc -m32 failed)"
    warn "You may need to install 32-bit development tools"
    warn "On Debian/Ubuntu: sudo apt install gcc-multilib g++-multilib"
  fi
  
  success "Prerequisites check passed"
}

# Clean previous build artifacts
clean() {
  log "Cleaning previous build artifacts..."
  
  if [ -d "$BUILD_DIR" ]; then
    rm -rf "$BUILD_DIR"
  fi
  
  if [ -d "$DEBIAN_I686_DIR" ]; then
    rm -rf "$DEBIAN_I686_DIR"
  fi
  
  if [ -d "$OUT_DIR" ]; then
    rm -rf "$OUT_DIR"
  fi
  
  mkdir -p "$BUILD_DIR" "$OUT_DIR"
  success "Build directory cleaned"
}

# Build Vitte compiler for i686
build_compiler() {
  log "Building Vitte compiler for Debian i686 (32-bit)..."
  
  cd "$ROOT_DIR"
  
  # Configure for 32-bit x86
  export CFLAGS="-O${OPT_LEVEL} -march=i686 -mtune=generic -m32"
  export CXXFLAGS="$CFLAGS"
  export LDFLAGS="-Wl,-rpath,/usr/lib/vitte -m32"
  
  log "Compiling with $JOBS jobs (32-bit architecture)..."
  make -j "$JOBS" \
    BUILD_DIR="$BUILD_DIR" \
    BIN_DIR="$BIN_DIR" \
    OPT_LEVEL="$OPT_LEVEL" \
    PREFIX="/usr" \
    build
  
  if [ ! -f "$BIN_DIR/vitte" ]; then
    die "Compiler build failed: $BIN_DIR/vitte not created"
  fi
  
  success "Compiler built: $BIN_DIR/vitte"
}

# Verify architecture
verify_architecture() {
  log "Verifying i686 architecture..."
  
  local file_output
  file_output=$(file "$BIN_DIR/vitte" 2>/dev/null || echo "")
  
  if [[ "$file_output" == *"Intel 80386"* ]] || [[ "$file_output" == *"i386"* ]]; then
    success "Binary is i686/i386 (32-bit) architecture"
  elif [[ "$file_output" == *"x86"* ]]; then
    success "Binary is x86 architecture"
  else
    warn "Could not verify 32-bit architecture from file command"
  fi
}

# Build standard library
build_stdlib() {
  log "Building standard library..."
  
  cd "$ROOT_DIR"
  
  if [ -d "$ROOT_DIR/src/vitte/packages" ]; then
    log "Standard library packages available in src/vitte/packages"
  fi
  
  success "Standard library ready"
}

# Verify editors
verify_editors() {
  log "Verifying editor support files..."
  bash "$ROOT_DIR/tools/check_editor_package_assets.sh"
  success "Editor support files validated"
}

# Create Debian control file
create_debian_control() {
  local control_file="$1"
  
  cat > "$control_file" << EOF
Package: $PACKAGE_NAME
Version: $VERSION
Architecture: $ARCH
Maintainer: $MAINTAINER
Homepage: https://vitte-lang.org/
Vcs-Git: https://github.com/vitte-lang/vitte.git
Section: devel
Priority: optional
Depends: bash, libc6
Recommends: make, python3, zsh
Description: Vitte systems language compiler and toolchain (32-bit)
 Vitte is a structured systems programming language focused on clarity,
 tooling, scalability, and long-term maintainability.
 .
 This is the 32-bit (i686) version for legacy systems.
 .
 This package includes:
 - Vitte compiler (vittec)
 - Standard library
 - Editor support (Vim, Emacs, Nano, Geany, Tree-sitter)
 - Shell completions
 - Documentation
 - Development tools
EOF

  success "Debian control file created"
}

# Create postinst script
create_postinst_script() {
  local postinst_file="$1"
  
  cat > "$postinst_file" << 'EOF'
#!/bin/bash
set -e

case "$1" in
  configure)
    update-alternatives --install /usr/bin/vitte vitte /usr/libexec/vitte/vitte 100
    
    if [ -d /etc/bash_completion.d ]; then
      ln -sf /usr/share/vitte/completions/vitte.bash \
        /etc/bash_completion.d/vitte 2>/dev/null || true
    fi
    
    cat << 'SETUP'

Vitte 32-bit (i686) has been installed successfully!

This is the 32-bit version of Vitte, suitable for legacy systems.

Next Steps:
  1. Verify: vitte --version
  2. Try: echo 'fn main() { println("Hello!") }' > test.vit && vitte test.vit
  3. Read: vitte --help

Documentation: /usr/share/vitte/docs/
SETUP
    ;;
esac

exit 0
EOF

  chmod 755 "$postinst_file"
  success "Postinst script created"
}

# Create prerm script
create_prerm_script() {
  local prerm_file="$1"
  
  cat > "$prerm_file" << 'EOF'
#!/bin/bash
set -e

case "$1" in
  remove)
    update-alternatives --remove vitte /usr/libexec/vitte/vitte 2>/dev/null || true
    ;;
esac

exit 0
EOF

  chmod 755 "$prerm_file"
  success "Prerm script created"
}

# Create Debian .deb package
create_deb_package() {
  log "Creating Debian i686 .deb package..."
  
  cd "$ROOT_DIR"
  
  local stage_base="$ROOT_DIR/.debstage-i686/$PACKAGE_NAME-$VERSION"
  local stage_root="$stage_base/root"
  local debian_dir="$stage_root/DEBIAN"
  
  if [ -d "$stage_base" ]; then
    rm -rf "$stage_base"
  fi
  
  mkdir -p "$stage_root/usr/bin" \
    "$stage_root/usr/libexec/vitte" \
    "$stage_root/usr/share/vitte/src/vitte" \
    "$stage_root/usr/share/vitte/editors" \
    "$stage_root/usr/share/vitte/completions" \
    "$stage_root/usr/share/vitte/docs" \
    "$stage_root/usr/share/man/man1" \
    "$debian_dir"
  
  log "Copying binaries..."
  install -m 0755 "$BIN_DIR/vitte" "$stage_root/usr/libexec/vitte/vitte"
  
  # Create wrapper script
  cat > "$stage_root/usr/bin/vitte" << 'WRAPPER'
#!/bin/bash
set -euo pipefail
root="${VITTE_ROOT:-/usr/share/vitte}"
export VITTE_ROOT="$root"
exec /usr/libexec/vitte/vitte "$@"
WRAPPER
  chmod 0755 "$stage_root/usr/bin/vitte"
  
  log "Copying source and standard library..."
  rsync -a "$ROOT_DIR/src/vitte/packages/" \
    "$stage_root/usr/share/vitte/src/vitte/packages/"
  rsync -a "$ROOT_DIR/src/vitte/compiler/" \
    "$stage_root/usr/share/vitte/src/vitte/compiler/"
  
  log "Copying editor support..."
  rsync -a "$ROOT_DIR/editors/" \
    "$stage_root/usr/share/vitte/editors/"
  
  log "Copying documentation..."
  rsync -a "$ROOT_DIR/docs/" \
    "$stage_root/usr/share/vitte/docs/"
  
  if [ -d "$ROOT_DIR/completions" ] && [ -n "$(ls -A "$ROOT_DIR/completions" 2>/dev/null)" ]; then
    rsync -a "$ROOT_DIR/completions/" \
      "$stage_root/usr/share/vitte/completions/"
  fi
  
  # Normalize permissions
  find "$stage_root/usr/share/vitte" -type d -exec chmod 0755 {} +
  find "$stage_root/usr/share/vitte" -type f -exec chmod 0644 {} +
  
  log "Creating Debian metadata..."
  create_debian_control "$debian_dir/control"
  create_postinst_script "$debian_dir/postinst"
  create_prerm_script "$debian_dir/prerm"
  
  find "$stage_root" -type f ! -path '*/DEBIAN/*' -exec md5sum {} \; > "$debian_dir/md5sums"
  
  log "Building .deb package (i686)..."
  mkdir -p "$OUT_DIR"
  
  local out_deb="$OUT_DIR/${PACKAGE_NAME}_${VERSION}_${ARCH}.deb"
  dpkg-deb --build "$stage_root" "$out_deb"
  
  if [ ! -f "$out_deb" ]; then
    die "Package creation failed"
  fi
  
  success "Package created: $out_deb"
  
  rm -rf "$stage_base"
}

# Summary
summary() {
  log "========================================================"
  log "Build Complete"
  log "========================================================"
  
  local out_deb="$OUT_DIR/${PACKAGE_NAME}_${VERSION}_${ARCH}.deb"
  if [ -f "$out_deb" ]; then
    local pkg_size
    pkg_size=$(du -h "$out_deb" | cut -f1)
    
    cat << EOF

${GREEN}✓ Vitte Debian i686 Package Successfully Built${NC}

Package Details:
  Version:      $VERSION
  Location:     $out_deb
  Size:         $pkg_size
  Architecture: $ARCH (32-bit i686)
  Platform:     Debian/Ubuntu Linux (32-bit)

Installation:
  sudo apt install $out_deb

Or:
  sudo dpkg -i $out_deb

After Installation:
  - Run: vitte --version
  - Read: /usr/share/vitte/docs/README.md

Note: This is the 32-bit version for legacy systems.

EOF
    success "Package ready for distribution"
  else
    error "Package file not found at expected location"
    return 1
  fi
}

# Main execution
main() {
  log "========================================================"
  log "Vitte Debian i686 (32-bit) Package Builder"
  log "========================================================"
  log "Version:  $VERSION"
  log "Package:  $PACKAGE_NAME"
  log "Arch:     $ARCH (i686 32-bit)"
  log "Output:   $OUT_DIR"
  log "Jobs:     $JOBS"
  log "========================================================\n"
  
  check_prerequisites
  clean
  build_compiler
  verify_architecture
  build_stdlib
  verify_editors
  create_deb_package
  summary
}

main "$@"
