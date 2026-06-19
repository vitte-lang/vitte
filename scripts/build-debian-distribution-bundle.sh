#!/usr/bin/env bash
# ============================================================
# build-debian-distribution-bundle.sh
#
# Create Debian distribution bundle with:
#   - .deb package
#   - Checksums (SHA256, SHA512)
#   - Installation guides
#   - Release notes
#   - Verification script
# ============================================================

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
OUT_DIR="${OUT_DIR:-$ROOT_DIR/pkgout}"
PKG_VERSION_FILE="$ROOT_DIR/toolchain/scripts/package/PACKAGE_VERSION"
VERSION="${VERSION:-$(tr -d ' \r\n' < "$PKG_VERSION_FILE" 2>/dev/null || echo 2.1.1)}"
ARCH="${ARCH:-amd64}"
PACKAGE_NAME="${PACKAGE_NAME:-vitte}"

log() { printf "${BLUE}[dist-bundle-deb]${NC} %s\n" "$*"; }
success() { printf "${GREEN}[dist-bundle-deb]${NC} ✓ %s\n" "$*"; }
error() { printf "${RED}[dist-bundle-deb][ERROR]${NC} %s\n" "$*" >&2; }
die() { error "$@"; exit 1; }

create_checksums() {
  log "Creating checksums for verification..."
  
  mkdir -p "$OUT_DIR"
  cd "$OUT_DIR"
  
  local deb_file="${PACKAGE_NAME}_${VERSION}_${ARCH}.deb"
  
  if [ -f "$deb_file" ]; then
    # SHA256
    sha256sum "$deb_file" > "${deb_file}.sha256"
    
    # SHA512
    sha512sum "$deb_file" > "${deb_file}.sha512"
    
    success "Checksums created"
    cat "${deb_file}.sha256"
  else
    die "Package file not found: $deb_file"
  fi
}

create_installation_guide() {
  log "Creating installation guide..."
  
  cat > "$OUT_DIR/INSTALL-DEBIAN.md" << 'EOF'
# Vitte Debian/Ubuntu Installation Guide

## System Requirements

- Debian 10+ or Ubuntu 18.04+
- x86_64 processor
- 500 MB free disk space

## Installation Methods

### Method 1: Using apt (Recommended)

```bash
sudo apt update
sudo dpkg -i vitte_*.deb
sudo apt install -f  # Install dependencies if needed
```

### Method 2: Direct dpkg

```bash
sudo dpkg -i vitte_*.deb
```

### Method 3: From .deb file

```bash
# Download the .deb file
sudo apt install ./vitte_*.deb
```

## Verification

After installation:

```bash
# Check version
vitte --version

# Check location
which vitte
/usr/bin/vitte

# Try compilation
echo 'fn main() { println("Hello!") }' > test.vit
vitte test.vit && ./test
```

## Environment Setup

### Shell Configuration

Add to your shell profile (`~/.bashrc`, `~/.zshrc`, etc.):

```bash
export PATH="/usr/bin:$PATH"
export VITTE_ROOT="/usr/share/vitte"
```

### Shell Completions

**Bash:**
```bash
source /usr/share/vitte/completions/vitte.bash
```

**Zsh:**
```bash
source /usr/share/vitte/completions/vitte.zsh
```

## Editor Setup

### Vim

Syntax files are installed to:
- `/usr/share/vitte/editors/vim/syntax/vitte.vim`

Add to `~/.vim/init.vim` or `~/.vimrc`:
```vim
set runtimepath+=/usr/share/vitte/editors/vim
```

### Emacs

Add to `~/.emacs.d/init.el`:
```elisp
(add-to-list 'load-path "/usr/share/vitte/editors/emacs")
(autoload 'vitte-mode "vitte-mode" nil t)
(add-to-list 'auto-mode-alist '("\\.vit\\'" . vitte-mode))
```

### Nano

Add to `~/.nanorc`:
```bash
include "/usr/share/vitte/editors/nano/vitte.nanorc"
```

### Geany

Configuration files are in:
- `/usr/share/vitte/editors/geany/`

## Uninstallation

To remove Vitte:

```bash
# Method 1: Using apt
sudo apt remove vitte

# Method 2: Using dpkg
sudo dpkg -r vitte
```

## Troubleshooting

### "Package not found" error

```bash
# Make sure the .deb file is in the correct location
ls -la vitte_*.deb

# Try installing with full path
sudo dpkg -i /path/to/vitte_*.deb
```

### Permission denied errors

```bash
# Ensure you're using sudo
sudo dpkg -i vitte_*.deb

# Or install to user directory with dpkg --root
```

### Dependency errors

```bash
# Install dependencies with apt
sudo apt install -f

# Or manually install dependencies
sudo apt install bash libc6
```

## Documentation

After installation, full documentation is available at:

```bash
open /usr/share/vitte/docs/

# Or from command line
ls /usr/share/vitte/docs/
```

## Support

For issues and questions:
- GitHub: https://github.com/vitte-lang/vitte
- Issues: https://github.com/vitte-lang/vitte/issues
- Documentation: https://vitte-lang.github.io/

## License

Vitte is licensed under the MIT License.
See `/usr/share/vitte/LICENSE` for details.
EOF

  success "Installation guide created"
}

create_quickstart() {
  log "Creating quickstart guide..."
  
  cat > "$OUT_DIR/QUICKSTART-DEBIAN.md" << 'EOF'
# Vitte on Debian/Ubuntu - Quick Start

## Installation

```bash
sudo dpkg -i vitte_*.deb
```

## Your First Program

### Hello World
```bash
cat > hello.vit << 'VITCODE'
fn main() {
    println("Hello, Vitte!")
}
VITCODE

vitte hello.vit
./hello
```

## Common Commands

```bash
# Show version
vitte --version

# Show help
vitte --help

# Compile a file
vitte myprogram.vit

# Check syntax without compiling
vitte --check myprogram.vit

# Run with debug info
vitte --debug myprogram.vit
```

## Shell Integration

### Bash

Add to `~/.bashrc`:
```bash
export VITTE_ROOT="/usr/share/vitte"
source /usr/share/vitte/completions/vitte.bash
```

### Zsh

Add to `~/.zshrc`:
```bash
export VITTE_ROOT="/usr/share/vitte"
source /usr/share/vitte/completions/vitte.zsh
```

Then:
```bash
source ~/.bashrc  # or ~/.zshrc
```

## Project Structure

```
myproject/
├── src/
│   ├── main.vit
│   ├── module1.vit
│   └── module2.vit
├── tests/
│   └── test_*.vit
└── README.md
```

## Documentation

- `/usr/share/vitte/docs/` - Full documentation
- `/usr/share/vitte/docs/README.md` - Getting started
- `vitte --help` - Command help
- https://vitte-lang.github.io/ - Online documentation

## Uninstall

```bash
sudo apt remove vitte
```

---

For more information, see the full installation guide or visit https://vitte-lang.github.io/
EOF

  success "Quickstart guide created"
}

create_release_notes() {
  log "Creating release notes..."
  
  cat > "$OUT_DIR/RELEASE-DEBIAN-${VERSION}.md" << EOF
# Vitte Language Compiler v${VERSION} - Debian/Ubuntu Release

**Release Date:** $(date -u +%Y-%m-%d)
**Platform:** Debian/Ubuntu Linux x86_64
**Architecture:** amd64

## Overview

This is the Debian/Ubuntu distribution package for Vitte, a structured systems programming language focused on clarity, tooling, scalability, and long-term maintainability.

## What's Included

### Core Components
- **Compiler** - Vitte language compiler (vitte)
- **Runtime** - Execution environment and support libraries
- **Standard Library** - Comprehensive standard library packages
- **Documentation** - Complete API and language documentation

### Editor Support
- **Vim** - Syntax highlighting, indentation, file detection
- **Emacs** - Full-featured Vitte mode with IDE support
- **Nano** - Lightweight syntax highlighting
- **Geany** - GTK+ IDE integration with snippets
- **Tree-sitter** - AST grammar for enhanced editors

### Developer Tools
- Shell completions for bash, zsh
- Standard library source code
- Debugging and profiling utilities

## Installation

### Quick Install

\`\`\`bash
sudo dpkg -i vitte_${VERSION}_amd64.deb
\`\`\`

### Verify Installation

\`\`\`bash
vitte --version
echo 'fn main() { println("Hello!") }' > test.vit
vitte test.vit && ./test
\`\`\`

## System Requirements

- Debian 10 (Buster) or later
- Ubuntu 18.04 or later
- x86_64 processor
- 500 MB free disk space

## Directory Structure

After installation:

\`\`\`
/usr/
├── bin/vitte              Main compiler
└── share/vitte/
    ├── src/               Source code & stdlib
    ├── docs/              Documentation
    ├── editors/           Editor support files
    ├── completions/       Shell completions
    └── ...
\`\`\`

## Supported Systems

- Debian 10 (Buster)
- Debian 11 (Bullseye)
- Debian 12 (Bookworm)
- Ubuntu 18.04 LTS
- Ubuntu 20.04 LTS
- Ubuntu 22.04 LTS
- Ubuntu 24.04 LTS
- Linux Mint
- Elementary OS
- Pop!_OS

## Getting Started

Create your first program:

\`\`\`bash
cat > hello.vit << 'VITCODE'
fn main() {
    println("Hello, Vitte!")
}
VITCODE

vitte hello.vit
./hello
\`\`\`

## Known Issues

- Requires glibc 2.28 or later
- Some features require additional system libraries

## Uninstallation

\`\`\`bash
sudo apt remove vitte
\`\`\`

## License

Vitte is licensed under the **MIT License**.

See \`/usr/share/vitte/LICENSE\` for complete details.

## Support

- **GitHub:** https://github.com/vitte-lang/vitte
- **Documentation:** https://vitte-lang.github.io/
- **Issues:** https://github.com/vitte-lang/vitte/issues

## Changelog

For version history, see:
- \`/usr/share/vitte/CHANGELOG\`
- https://github.com/vitte-lang/vitte/blob/main/CHANGELOG.md

---

**Build Information:**
- Version: ${VERSION}
- Platform: Debian/Ubuntu Linux
- Architecture: x86_64 (amd64)
- Built: $(date -u +%Y-%m-%dT%H:%M:%SZ)
EOF

  success "Release notes created"
}

create_verification_script() {
  log "Creating verification script..."
  
  cat > "$OUT_DIR/verify-debian.sh" << 'EOF'
#!/usr/bin/env bash
set -euo pipefail

ARCH="${1:-amd64}"
PACKAGE_NAME="${2:-vitte}"
VERSION="${3:-.}"

DEB_FILE="${PACKAGE_NAME}_${VERSION}_${ARCH}.deb"
SHA256_FILE="${DEB_FILE}.sha256"

if [ ! -f "$SHA256_FILE" ]; then
  echo "Error: $SHA256_FILE not found"
  exit 1
fi

echo "Verifying $DEB_FILE..."

if sha256sum -c "$SHA256_FILE"; then
  echo "✓ Package verification successful!"
  exit 0
else
  echo "✗ Package verification failed!"
  exit 1
fi
EOF

  chmod +x "$OUT_DIR/verify-debian.sh"
  success "Verification script created"
}

create_package_manifest() {
  log "Creating package manifest..."
  
  cat > "$OUT_DIR/PACKAGE_MANIFEST-DEBIAN.txt" << EOF
Vitte Debian x86_64 Package v${VERSION}
========================================

Generated: $(date -u +%Y-%m-%dT%H:%M:%SZ)
Architecture: amd64 (x86_64)
Platform: Debian/Ubuntu Linux

Package Installation:
  dpkg -i vitte_${VERSION}_amd64.deb
  sudo apt install -f  # Install dependencies

Installed Paths:
------------------

Binaries:
  /usr/bin/vitte                Main compiler
  /usr/libexec/vitte/vitte      Compiler binary

Source & Libraries:
  /usr/share/vitte/src/vitte/packages/    Source packages
  /usr/share/vitte/src/vitte/stdlib/      Standard library
  /usr/share/vitte/src/vitte/compiler/    Compiler source
  /usr/share/vitte/src/compiler/backends/ Backend sources

Editor Support:
  /usr/share/vitte/editors/vim/           Vim support
  /usr/share/vitte/editors/emacs/         Emacs support
  /usr/share/vitte/editors/nano/          Nano support
  /usr/share/vitte/editors/geany/         Geany support
  /usr/share/vitte/editors/tree-sitter/   Tree-sitter grammar

Documentation:
  /usr/share/vitte/docs/       Full documentation
  /usr/share/vitte/CHANGELOG   Version history
  /usr/share/vitte/LICENSE     License file

Utilities:
  /usr/share/vitte/completions/  Shell completions
  /usr/share/vitte/env.sh        Environment setup

Package Details:
  Maintainer: Vitte Team <maintainers@vitte-lang.org>
  Homepage: https://vitte-lang.org/
  Repository: https://github.com/vitte-lang/vitte
  License: MIT

Dependencies:
  bash, libc6

Recommended Packages:
  make, python3, zsh

Installation Size: ~150-250 MB
Disk Space Required: ~500 MB

For installation instructions, see: INSTALL-DEBIAN.md
For quick start, see: QUICKSTART-DEBIAN.md
EOF

  success "Package manifest created"
}

main() {
  log "========================================================"
  log "Creating Debian Distribution Bundle"
  log "========================================================"
  log "Version: $VERSION"
  log "Arch:    $ARCH"
  log "Output:  $OUT_DIR"
  log "========================================================\n"
  
  mkdir -p "$OUT_DIR"
  
  create_checksums
  create_installation_guide
  create_quickstart
  create_release_notes
  create_verification_script
  create_package_manifest
  
  log "========================================================"
  log "Distribution Bundle Complete"
  log "========================================================"
  log ""
  log "Files in $OUT_DIR:"
  ls -lh "$OUT_DIR" | tail -n +2 | awk '{print "  " $9 " (" $5 ")"}'
  log ""
  success "Bundle ready for distribution"
}

main "$@"
