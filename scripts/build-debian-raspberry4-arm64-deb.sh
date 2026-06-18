#!/usr/bin/env bash
# ============================================================
# build-debian-raspberry4-arm64-deb.sh
#
# Complete build pipeline for Vitte Debian/Raspberry Pi 4 ARM64 package
# Includes:
#   - Compiler compilation (ARM64 / aarch64 / Cortex-A72)
#   - Standard library
#   - Editors (vim, emacs, nano, geany, tree-sitter)
#   - Documentation
#   - Completions
#   - .deb installer creation
#
# Target:
#   Raspberry Pi 4, Raspberry Pi OS 64-bit, Debian ARM64, Ubuntu ARM64
#
# Output:
#   pkgout/vitte_*_arm64.deb
#
# Usage:
#   chmod +x toolchain/scripts/package/build-debian-raspberry4-arm64-deb.sh
#   ./toolchain/scripts/package/build-debian-raspberry4-arm64-deb.sh
#
# Optional overrides:
#   VERSION=2.1.1 OPT_LEVEL=2 JOBS=4 ./build-debian-raspberry4-arm64-deb.sh
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
DEBIAN_ARM64_DIR="${DEBIAN_ARM64_DIR:-$TARGET_DIR/debian-raspberry4-arm64}"
OUT_DIR="${OUT_DIR:-$ROOT_DIR/pkgout}"

PKG_VERSION_FILE="$ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION"
VERSION="${VERSION:-$(tr -d ' \r\n' < "$PKG_VERSION_FILE" 2>/dev/null || echo 2.1.1)}"
ARCH="${ARCH:-arm64}"
PACKAGE_NAME="${PACKAGE_NAME:-vitte}"
MAINTAINER="${MAINTAINER:-Vitte Team <maintainers@vitte-lang.org>}"

JOBS="${JOBS:-$(nproc 2>/dev/null || echo 4)}"
OPT_LEVEL="${OPT_LEVEL:-2}"
TARGET_CPU="${TARGET_CPU:-cortex-a72}"
TARGET_PLATFORM="${TARGET_PLATFORM:-raspberrypi4-arm64}"

# Helpers
log() {
  printf "${BLUE}[build-rpi4-deb]${NC} %s\n" "$*"
}

success() {
  printf "${GREEN}[build-rpi4-deb]${NC} ✓ %s\n" "$*"
}

warn() {
  printf "${YELLOW}[build-rpi4-deb]${NC} ⚠ %s\n" "$*"
}

error() {
  printf "${RED}[build-rpi4-deb][ERROR]${NC} %s\n" "$*" >&2
}

die() {
  error "$@"
  exit 1
}

# Check prerequisites
check_prerequisites() {
  log "Checking prerequisites for Raspberry Pi 4 ARM64 Debian build..."

  if [[ "${OSTYPE:-}" != "linux-gnu"* ]]; then
    die "This script requires Linux"
  fi

  local required_tools=("make" "dpkg-deb" "rsync" "python3" "file")
  for tool in "${required_tools[@]}"; do
    if ! command -v "$tool" >/dev/null 2>&1; then
      die "Required tool not found: $tool"
    fi
  done

  local build_tools=("gcc" "g++")
  for tool in "${build_tools[@]}"; do
    if ! command -v "$tool" >/dev/null 2>&1; then
      warn "Build tool not found: $tool"
    fi
  done

  if command -v dpkg >/dev/null 2>&1; then
    local host_arch
    host_arch="$(dpkg --print-architecture 2>/dev/null || true)"
    if [ -n "$host_arch" ] && [ "$host_arch" != "arm64" ]; then
      warn "Host Debian architecture is '$host_arch', package target is 'arm64'"
      warn "Use this natively on Raspberry Pi OS 64-bit, or configure a cross toolchain externally"
    fi
  fi

  success "Prerequisites check passed"
}

# Clean previous build artifacts
clean() {
  log "Cleaning previous Raspberry Pi 4 ARM64 build artifacts..."

  rm -rf "$BUILD_DIR" "$DEBIAN_ARM64_DIR" "$OUT_DIR"
  mkdir -p "$BUILD_DIR" "$OUT_DIR"

  success "Build directory cleaned"
}

# Build Vitte compiler for Raspberry Pi 4 ARM64
build_compiler() {
  log "Building Vitte compiler for Raspberry Pi 4 ARM64..."

  cd "$ROOT_DIR"

  export CFLAGS="-O${OPT_LEVEL} -mcpu=${TARGET_CPU} -pipe"
  export CXXFLAGS="$CFLAGS"
  export LDFLAGS="-Wl,-rpath,/usr/lib/vitte"
  export VITTE_TARGET_ARCH="aarch64"
  export VITTE_TARGET_PLATFORM="$TARGET_PLATFORM"

  log "CFLAGS: $CFLAGS"
  log "Compiling with $JOBS jobs..."

  make -j "$JOBS" \
    BUILD_DIR="$BUILD_DIR" \
    BIN_DIR="$BIN_DIR" \
    OPT_LEVEL="$OPT_LEVEL" \
    PREFIX="/usr" \
    TARGET_ARCH="aarch64" \
    TARGET_CPU="$TARGET_CPU" \
    build

  if [ ! -f "$BIN_DIR/vitte" ]; then
    die "Compiler build failed: $BIN_DIR/vitte not created"
  fi

  success "Compiler built: $BIN_DIR/vitte"
}

# Verify architecture
verify_architecture() {
  log "Verifying ARM64/aarch64 architecture..."

  local file_output
  file_output="$(file "$BIN_DIR/vitte" 2>/dev/null || echo "")"
  log "file: $file_output"

  if [[ "$file_output" == *"aarch64"* ]] || [[ "$file_output" == *"ARM aarch64"* ]] || [[ "$file_output" == *"ARM64"* ]]; then
    success "Binary is ARM64/aarch64 architecture"
  elif [[ "$file_output" == *"ARM"* ]]; then
    warn "Binary is ARM, but not clearly ARM64/aarch64"
  else
    warn "Could not verify ARM64 architecture from file command"
  fi
}

# Build standard library
build_stdlib() {
  log "Preparing standard library..."

  cd "$ROOT_DIR"

  if [ -d "$ROOT_DIR/src/vitte/packages" ]; then
    success "Standard library packages found: src/vitte/packages"
  else
    warn "Standard library packages not found: src/vitte/packages"
  fi

  if [ -d "$ROOT_DIR/src/vitte/compiler" ]; then
    success "Compiler sources found: src/vitte/compiler"
  else
    warn "Compiler sources not found: src/vitte/compiler"
  fi
}

# Verify editors
verify_editors() {
  log "Verifying editor support files..."
  bash "$ROOT_DIR/tools/check_editor_package_assets.sh"
  success "Editor support files validated"
}

# Safe rsync helper
copy_dir_if_exists() {
  local src="$1"
  local dst="$2"
  local label="$3"

  if [ -d "$src" ]; then
    mkdir -p "$dst"
    rsync -a "$src/" "$dst/"
    success "Copied $label"
  else
    warn "Skipped $label: $src not found"
  fi
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
Description: Vitte systems language compiler and toolchain for Raspberry Pi 4
 Vitte is a structured systems programming language focused on clarity,
 tooling, scalability, and long-term maintainability.
 .
 This package targets Raspberry Pi 4 ARM64/aarch64 and includes:
 - Vitte compiler
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

    if [ -d /etc/bash_completion.d ] && [ -f /usr/share/vitte/completions/vitte.bash ]; then
      ln -sf /usr/share/vitte/completions/vitte.bash \
        /etc/bash_completion.d/vitte 2>/dev/null || true
    fi

    cat << 'SETUP'

Vitte has been installed successfully for Raspberry Pi 4 ARM64.

Editor integrations are available under /usr/share/vitte/editors/.

Next Steps:
  1. Verify installation: vitte --version
  2. Test compiler: vitte --help
  3. Read documentation: /usr/share/vitte/docs/

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
  log "Creating Raspberry Pi 4 ARM64 Debian .deb package..."

  cd "$ROOT_DIR"

  local stage_base="$ROOT_DIR/.debstage/$PACKAGE_NAME-$VERSION-$ARCH"
  local stage_root="$stage_base/root"
  local debian_dir="$stage_root/DEBIAN"

  rm -rf "$stage_base"

  mkdir -p "$stage_root/usr/bin" \
    "$stage_root/usr/libexec/vitte" \
    "$stage_root/usr/share/vitte/src/vitte/packages" \
    "$stage_root/usr/share/vitte/src/vitte/compiler" \
    "$stage_root/usr/share/vitte/editors" \
    "$stage_root/usr/share/vitte/completions" \
    "$stage_root/usr/share/vitte/docs" \
    "$stage_root/usr/share/man/man1" \
    "$debian_dir"

  log "Copying binary..."
  install -m 0755 "$BIN_DIR/vitte" "$stage_root/usr/libexec/vitte/vitte"

  cat > "$stage_root/usr/bin/vitte" << 'WRAPPER'
#!/bin/bash
set -euo pipefail
root="${VITTE_ROOT:-/usr/share/vitte}"
export VITTE_ROOT="$root"
exec /usr/libexec/vitte/vitte "$@"
WRAPPER
  chmod 0755 "$stage_root/usr/bin/vitte"

  log "Copying source, standard library, editors, docs and completions..."
  copy_dir_if_exists "$ROOT_DIR/src/vitte/packages" "$stage_root/usr/share/vitte/src/vitte/packages" "standard library"
  copy_dir_if_exists "$ROOT_DIR/src/vitte/compiler" "$stage_root/usr/share/vitte/src/vitte/compiler" "compiler sources"
  copy_dir_if_exists "$ROOT_DIR/editors" "$stage_root/usr/share/vitte/editors" "editor support"
  copy_dir_if_exists "$ROOT_DIR/docs" "$stage_root/usr/share/vitte/docs" "documentation"
  copy_dir_if_exists "$ROOT_DIR/completions" "$stage_root/usr/share/vitte/completions" "shell completions"

  find "$stage_root/usr/share/vitte" -type d -exec chmod 0755 {} +
  find "$stage_root/usr/share/vitte" -type f -exec chmod 0644 {} +

  log "Creating Debian metadata..."
  create_debian_control "$debian_dir/control"
  create_postinst_script "$debian_dir/postinst"
  create_prerm_script "$debian_dir/prerm"

  (cd "$stage_root" && find . -type f ! -path './DEBIAN/*' -exec md5sum {} \;) > "$debian_dir/md5sums"

  log "Building .deb package..."
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
    pkg_size="$(du -h "$out_deb" | cut -f1)"

    cat << EOF

${GREEN}✓ Vitte Raspberry Pi 4 ARM64 Debian Package Successfully Built${NC}

Package Details:
  Version:       $VERSION
  Location:      $out_deb
  Size:          $pkg_size
  Architecture:  $ARCH
  CPU Target:    $TARGET_CPU
  Platform:      Raspberry Pi 4 / Debian ARM64 / Raspberry Pi OS 64-bit

Installation on Raspberry Pi 4:
  sudo apt install "$out_deb"

Or:
  sudo dpkg -i "$out_deb"
  sudo apt -f install

After Installation:
  vitte --version
  vitte --help

EOF
    success "Package ready for Raspberry Pi 4 distribution"
  else
    error "Package file not found at expected location"
    return 1
  fi
}

# Main execution
main() {
  log "========================================================"
  log "Vitte Debian Raspberry Pi 4 ARM64 Package Builder"
  log "========================================================"
  log "Version:   $VERSION"
  log "Package:   $PACKAGE_NAME"
  log "Arch:      $ARCH"
  log "CPU:       $TARGET_CPU"
  log "Output:    $OUT_DIR"
  log "Jobs:      $JOBS"
  log "========================================================"

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
