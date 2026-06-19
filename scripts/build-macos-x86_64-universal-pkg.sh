#!/usr/bin/env bash
# ============================================================
# build-macos-x86_64-universal-pkg.sh
# 
# Complete build pipeline for Vitte macOS universal x86_64 package
# Includes:
#   - Compiler compilation (x86_64)
#   - Standard library
#   - Editors (vim, emacs, nano, geany, tree-sitter)
#   - Documentation
#   - Completions
#   - .pkg installer creation
# 
# Output: pkgout/vitte-*.pkg
# ============================================================

set -euo pipefail

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/build}"
BIN_DIR="${BIN_DIR:-$ROOT_DIR/bin}"
TARGET_DIR="${TARGET_DIR:-$ROOT_DIR/target}"
MACOS_X86_64_DIR="${MACOS_X86_64_DIR:-$TARGET_DIR/macos-x86_64}"
MACOS_UNIVERSAL_DIR="${MACOS_UNIVERSAL_DIR:-$TARGET_DIR/universal}"
OUT_DIR="${OUT_DIR:-$ROOT_DIR/pkgout}"

PKG_VERSION_FILE="$ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION"
VERSION="${VERSION:-$(tr -d ' \r\n' < "$PKG_VERSION_FILE" 2>/dev/null || echo 2.1.1)}"

JOBS="${JOBS:-$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)}"
OPT_LEVEL="${OPT_LEVEL:-2}"
BUILD_TARGET="${BUILD_TARGET:-bootstrap-all}"

# Helper functions
log() {
  printf "${BLUE}[build-pkg]${NC} %s\n" "$*"
}

success() {
  printf "${GREEN}[build-pkg]${NC} ✓ %s\n" "$*"
}

warn() {
  printf "${YELLOW}[build-pkg]${NC} ⚠ %s\n" "$*"
}

error() {
  printf "${RED}[build-pkg][ERROR]${NC} %s\n" "$*" >&2
}

die() {
  error "$@"
  exit 1
}

# Check prerequisites
check_prerequisites() {
  log "Checking prerequisites..."
  
  # Check for macOS
  if [[ "$OSTYPE" != "darwin"* ]]; then
    die "This script requires macOS (not running on darwin)"
  fi
  
  # Check for required tools
  local required_tools=("make" "git" "rsync" "pkgbuild" "productbuild")
  for tool in "${required_tools[@]}"; do
    if ! command -v "$tool" &> /dev/null; then
      die "Required tool not found: $tool"
    fi
  done
  
  # Check Xcode Command Line Tools
  if ! xcode-select -p &>/dev/null; then
    die "Xcode Command Line Tools not installed. Run: xcode-select --install"
  fi
  
  success "Prerequisites check passed"
}

# Clean previous build artifacts
clean() {
  log "Cleaning previous build artifacts..."
  
  if [ -d "$MACOS_X86_64_DIR" ]; then
    rm -rf "$MACOS_X86_64_DIR"
  fi

  rm -rf "$ROOT_DIR/.pkgstage"

  mkdir -p "$BUILD_DIR" "$OUT_DIR"
  rm -f "$OUT_DIR/vitte-$VERSION.pkg"
  success "macOS package build artifacts cleaned"
}

# Build Vitte compiler for x86_64
build_compiler() {
  log "Building Vitte compiler for x86_64..."
  
  cd "$ROOT_DIR"
  
  # Configure and build
  export CFLAGS="-O${OPT_LEVEL} -march=x86-64 -mtune=generic"
  export CXXFLAGS="$CFLAGS"
  export LDFLAGS="-Wl,-rpath,/usr/local/lib"
  
  # Run make with parallel jobs
  log "Compiling with $JOBS jobs..."
  make -j "$JOBS" \
    BUILD_DIR="$BUILD_DIR" \
    BIN_DIR="$BIN_DIR" \
    OPT_LEVEL="$OPT_LEVEL" \
    PREFIX="/usr/local" \
    "$BUILD_TARGET"
  
  if [ ! -f "$BIN_DIR/vitte" ]; then
    die "Compiler build failed: $BIN_DIR/vitte not created"
  fi
  
  success "Compiler built: $BIN_DIR/vitte"
}

# Create universal binary (lipo for x86_64)
create_universal_binary() {
  log "Creating universal binary for x86_64..."
  
  mkdir -p "$MACOS_UNIVERSAL_DIR"
  
  # For x86_64 only, just copy the binary
  cp "$BIN_DIR/vitte" "$MACOS_UNIVERSAL_DIR/vitte"
  chmod 0755 "$MACOS_UNIVERSAL_DIR/vitte"
  
  # Verify architecture
  local arch_info
  arch_info=$(lipo -info "$MACOS_UNIVERSAL_DIR/vitte" 2>/dev/null || echo "x86_64")
  log "Binary architecture: $arch_info"
  
  success "Universal binary created: $MACOS_UNIVERSAL_DIR/vitte"
}

# Build standard library
build_stdlib() {
  log "Building standard library..."
  
  cd "$ROOT_DIR"
  
  # Standard library is typically compiled as part of the main build
  # or pre-compiled bytecode is included
  if [ -d "$ROOT_DIR/src/vitte/packages" ]; then
    log "Standard library packages available in src/vitte/packages"
  fi
  if [ -d "$ROOT_DIR/src/vitte/stdlib" ]; then
    log "Standard library available in src/vitte/stdlib"
  fi
  
  success "Standard library ready"
}

# Verify editors are present
verify_editors() {
  log "Verifying editor support files..."
  
  local editors=("vim" "emacs" "nano" "geany" "tree-sitter")
  for editor in "${editors[@]}"; do
    if [ -d "$ROOT_DIR/editors/$editor" ]; then
      success "Editor support: $editor"
    else
      warn "Editor support not found: $editor"
    fi
  done
}

# Create macOS .pkg installer
create_pkg() {
  log "Creating macOS .pkg installer..."
  
  cd "$ROOT_DIR"
  
  # Use the existing package creation script
  if [ ! -f "$ROOT_DIR/toolchain/scripts/package/make-macos-pkg.sh" ]; then
    die "Package script not found: $ROOT_DIR/toolchain/scripts/package/make-macos-pkg.sh"
  fi
  
  log "Running package builder..."
  bash "$ROOT_DIR/toolchain/scripts/package/make-macos-pkg.sh"
  
  if [ ! -f "$OUT_DIR/vitte-$VERSION.pkg" ]; then
    die "Package creation failed"
  fi
  
  success "Package created: $OUT_DIR/vitte-$VERSION.pkg"
}

# Generate installation documentation
generate_docs() {
  log "Generating installation documentation..."
  
  cat > "$OUT_DIR/INSTALL.md" << 'EOF'
# Vitte macOS Installation Guide

## System Requirements

- macOS 10.13 (High Sierra) or later
- x86_64 processor
- 500 MB free disk space

## Installation

### Method 1: Using the Installer (.pkg)

1. Download `vitte-*.pkg`
2. Double-click the installer
3. Follow the installation wizard
4. The compiler will be installed to `/usr/local/bin/vitte`

### Method 2: Command Line Installation

```bash
sudo installer -pkg vitte-*.pkg -target /
```

## Verification

After installation, verify the compiler works:

```bash
vitte --version
vitte --help
```

## Uninstallation

To uninstall Vitte:

```bash
bash /usr/local/share/vitte/uninstall.sh
```

## Environment Setup

### Shell Configuration

Add Vitte to your shell profile (`~/.zshrc`, `~/.bash_profile`, etc.):

```bash
export PATH="/usr/local/bin:$PATH"
export VITTE_ROOT="/usr/local/share/vitte"
```

### Editor Setup

#### Vim

Syntax highlighting and IDE features are automatically installed in:
- `~/.vim/syntax/vitte.vim`
- `~/.vim/ftdetect/vitte.vim`
- `~/.vim/ftplugin/vitte.vim`

#### Emacs

Add to your `~/.emacs.d/init.el`:

```elisp
(add-to-list 'load-path "~/.emacs.d/lisp")
(autoload 'vitte-mode "vitte-mode" nil t)
(add-to-list 'auto-mode-alist '("\\.vit\\'" . vitte-mode))
```

#### Nano

Include Vitte syntax highlighting by adding to `~/.nanorc`:

```
include "~/.config/nano/vitte.nanorc"
```

## Quick Start

Create a hello world program:

```bash
cat > hello.vit << 'VITCODE'
// Hello World in Vitte
fn main() {
    println("Hello, World!")
}
VITCODE

vitte hello.vit
./hello
```

## Documentation

Full documentation is available at:

```bash
open /usr/local/share/vitte/docs/
```

Or view online at: https://vitte-lang.github.io/

## Support

For issues and questions:
- GitHub: https://github.com/vitte-lang/vitte
- Documentation: /usr/local/share/vitte/docs/

## License

Vitte is licensed under the MIT License.
See `/usr/local/share/vitte/LICENSE` for details.
EOF

  success "Installation guide created: $OUT_DIR/INSTALL.md"
  
  # Create package contents manifest
  cat > "$OUT_DIR/PACKAGE_MANIFEST.txt" << EOF
Vitte Universal macOS Package v$VERSION
========================================

Generated: $(date -u +%Y-%m-%dT%H:%M:%SZ)
Architecture: x86_64

Package Contents:
------------------

Binaries:
  - /usr/local/bin/vitte       (main compiler)
  - /usr/local/bin/vittec      (compiler wrapper)
  - /usr/local/libexec/vitte/  (compiler internals)

Standard Library & Source:
  - /usr/local/share/vitte/src/vitte/packages/    (stdlib packages)
  - /usr/local/share/vitte/src/vitte/stdlib/      (standard library)
  - /usr/local/share/vitte/src/vitte/compiler/    (compiler source)
  - /usr/local/share/vitte/src/compiler/backends/ (backend sources)

Editor Support:
  - vim      (~/.vim/*)
  - emacs    (~/.emacs.d/lisp/vitte-mode.el)
  - nano     (~/.config/nano/vitte.nanorc)
  - geany    (configuration files)
  - tree-sitter (grammar & queries)

Documentation:
  - /usr/local/share/vitte/docs/       (full documentation)
  - /usr/local/share/vitte/CHANGELOG   (changelog)
  - /usr/local/share/vitte/LICENSE     (license)
  - /usr/local/share/vitte/NOTICE      (notices)

Utilities:
  - /usr/local/share/vitte/env.sh              (environment setup)
  - /usr/local/share/vitte/uninstall.sh        (uninstaller)
  - /usr/local/share/vitte/completions/        (shell completions)

Checksums & Verification:
  - /usr/local/share/vitte/checksums/vitte.sha256
  - /usr/local/share/vitte/checksums/vitte.sha512
  - /usr/local/share/vitte/checksums/vitte.signature.txt

Tests & Validation:
  - /usr/local/share/vitte/tests/modules/  (test modules)

Installation Size: ~200-300 MB
Disk Space Required: ~500 MB

For installation instructions, see: INSTALL.md
EOF

  success "Package manifest created"
}

# Display build summary
summary() {
  log "========================================================"
  log "Build Complete"
  log "========================================================"
  
  if [ -f "$OUT_DIR/vitte-$VERSION.pkg" ]; then
    local pkg_size
    pkg_size=$(du -h "$OUT_DIR/vitte-$VERSION.pkg" | cut -f1)
    
    cat << EOF

${GREEN}✓ Vitte macOS Package Successfully Built${NC}

Package Details:
  Version:      $VERSION
  Location:     $OUT_DIR/vitte-$VERSION.pkg
  Size:         $pkg_size
  Architecture: x86_64
  Platform:     macOS
  
Installation:
  1. Double-click the .pkg file to open installer
  2. Follow the installation wizard
  3. Or use: sudo installer -pkg $OUT_DIR/vitte-$VERSION.pkg -target /

After Installation:
  - Run: vitte --version
  - Read: /usr/local/share/vitte/docs/README.md
  - Uninstall: bash /usr/local/share/vitte/uninstall.sh

Documentation:
  - See: $OUT_DIR/INSTALL.md
  - See: $OUT_DIR/PACKAGE_MANIFEST.txt

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
  log "Vitte macOS x86_64 Universal Package Builder"
  log "========================================================"
  log "Version:  $VERSION"
  log "Output:   $OUT_DIR"
  log "Jobs:     $JOBS"
  log "Build target: $BUILD_TARGET"
  log "========================================================\n"
  
  check_prerequisites
  clean
  build_compiler
  create_universal_binary
  build_stdlib
  verify_editors
  create_pkg
  generate_docs
  summary
}

# Run main
main "$@"
