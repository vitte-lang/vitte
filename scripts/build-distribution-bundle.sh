#!/usr/bin/env bash
# ============================================================
# build-distribution-bundle.sh
#
# Create a complete distribution bundle with:
#   - Compiled universal x86_64 binary
#   - Compressed DMG image (optional)
#   - Checksum files for verification
#   - Release notes
#   - Installation guides
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
CREATE_DMG="${CREATE_DMG:-0}"

log() { printf "${BLUE}[dist-bundle]${NC} %s\n" "$*"; }
success() { printf "${GREEN}[dist-bundle]${NC} ✓ %s\n" "$*"; }
error() { printf "${RED}[dist-bundle][ERROR]${NC} %s\n" "$*" >&2; }
die() { error "$@"; exit 1; }

create_checksums() {
  log "Creating checksums for verification..."
  
  mkdir -p "$OUT_DIR"
  cd "$OUT_DIR"
  
  if [ -f "vitte-$VERSION.pkg" ]; then
    # SHA256
    if command -v sha256sum &>/dev/null; then
      sha256sum "vitte-$VERSION.pkg" > "vitte-$VERSION.pkg.sha256"
    else
      shasum -a 256 "vitte-$VERSION.pkg" > "vitte-$VERSION.pkg.sha256"
    fi
    
    # SHA512
    if command -v sha512sum &>/dev/null; then
      sha512sum "vitte-$VERSION.pkg" > "vitte-$VERSION.pkg.sha512"
    else
      shasum -a 512 "vitte-$VERSION.pkg" > "vitte-$VERSION.pkg.sha512"
    fi
    
    success "Checksums created"
    cat "vitte-$VERSION.pkg.sha256"
  fi
}

create_dmg() {
  log "Creating DMG image..."
  
  local dmg_name="vitte-$VERSION.dmg"
  local dmg_temp="$OUT_DIR/vitte-temp.dmg"
  local dmg_mount="/Volumes/Vitte"
  
  # Create temporary DMG
  hdiutil create -volname "Vitte" \
    -srcfolder "$OUT_DIR" \
    -ov -format UDZO \
    -o "$dmg_temp" 2>/dev/null || true
  
  mv "$dmg_temp.dmg" "$OUT_DIR/$dmg_name" 2>/dev/null || {
    warn "DMG creation skipped (may require root)"
    return 1
  }
  
  success "DMG image created: $dmg_name"
}

create_release_notes() {
  log "Creating release notes..."
  
  cat > "$OUT_DIR/RELEASE-$VERSION.md" << EOF
# Vitte Language Compiler v$VERSION - Release Notes

**Release Date:** $(date -u +%Y-%m-%d)

## Overview

This is the macOS x86_64 distribution package for Vitte, a structured systems programming language focused on clarity, tooling, scalability, and long-term maintainability.

## What's Included

### Core Components
- **Compiler** - Vitte language compiler (vittec)
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
- Man pages
- Debugging and profiling utilities
- Standard library source code

## System Requirements

- macOS 10.13 (High Sierra) or later
- x86_64 processor (Intel/AMD)
- 500 MB free disk space
- 200-300 MB for full installation

## Installation

### Quick Install

1. Download \`vitte-$VERSION.pkg\`
2. Double-click to open the installer
3. Follow the installation wizard
4. Binaries installed to \`/usr/local/bin/\`

### Command Line Install

\`\`\`bash
sudo installer -pkg vitte-$VERSION.pkg -target /
\`\`\`

### Verify Installation

\`\`\`bash
vitte --version
echo 'fn main() { println("Hello!") }' > test.vit
vitte test.vit && ./test
\`\`\`

## Directory Structure

After installation, Vitte will be organized as:

\`\`\`
/usr/local/
├── bin/
│   ├── vitte      (main compiler)
│   └── vittec     (wrapper)
└── share/vitte/
    ├── src/       (source code & standard library)
    ├── docs/      (documentation)
    ├── editors/   (editor support files)
    └── tests/     (test modules)
\`\`\`

## Getting Started

### Create Your First Program

\`\`\`bash
cat > hello.vit << 'VITCODE'
// Simple Hello World
fn main() {
    println("Hello, Vitte!")
}
VITCODE

vitte hello.vit
./hello
\`\`\`

### Documentation

Full documentation is available in:
- \`/usr/local/share/vitte/docs/\`
- Online: https://vitte-lang.github.io/

### Shell Completion

To enable shell completions:

**Bash:**
\`\`\`bash
source /usr/local/share/vitte/completions/vitte.bash
\`\`\`

**Zsh:**
\`\`\`bash
source /usr/local/share/vitte/completions/vitte.zsh
\`\`\`

## Known Issues & Limitations

- macOS 10.12 and earlier are not supported
- Requires Xcode Command Line Tools for C code compilation
- Some advanced features require system libraries

## Uninstallation

To completely remove Vitte:

\`\`\`bash
bash /usr/local/share/vitte/uninstall.sh
\`\`\`

Or manually:

\`\`\`bash
sudo rm -rf /usr/local/bin/vitte /usr/local/bin/vittec /usr/local/share/vitte
\`\`\`

## License

Vitte is licensed under the **MIT License**.

See \`/usr/local/share/vitte/LICENSE\` for complete license text.

## Support & Community

- **GitHub:** https://github.com/vitte-lang/vitte
- **Documentation:** https://vitte-lang.github.io/
- **Issues & Discussions:** https://github.com/vitte-lang/vitte/discussions

## Changelog

For detailed changes in this version, see:
\`/usr/local/share/vitte/CHANGELOG\`

Or visit: https://github.com/vitte-lang/vitte/blob/main/CHANGELOG.md

---

**Verification:** To verify this package's integrity, check the SHA256 signature:

\`\`\`bash
shasum -a 256 -c vitte-$VERSION.pkg.sha256
\`\`\`

**Build Information:**
- Version: $VERSION
- Platform: macOS
- Architecture: x86_64
- Built: $(date -u +%Y-%m-%dT%H:%M:%SZ)
EOF

  success "Release notes created: RELEASE-$VERSION.md"
}

create_quickstart() {
  log "Creating quickstart guide..."
  
  cat > "$OUT_DIR/QUICKSTART.md" << 'EOF'
# Vitte Quick Start Guide

## Installation (macOS)

```bash
# 1. Open the installer
sudo installer -pkg vitte-*.pkg -target /

# 2. Verify installation
vitte --version
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

### With Variables
```vitte
fn main() {
    let name = "Vitte"
    let version = 2
    println("Welcome to ${name} v${version}")
}
```

### Using Standard Library
```vitte
// Access standard library
import std.io
import std.fmt

fn main() {
    let numbers = [1, 2, 3, 4, 5]
    for n in numbers {
        fmt.println("Number: {}", n)
    }
}
```

## Editor Setup

### Vim
Automatically installed to ~/.vim/
Features:
- Syntax highlighting
- Auto-indentation
- File type detection

### Emacs
Add to ~/.emacs.d/init.el:
```elisp
(add-to-list 'load-path "~/.emacs.d/lisp")
(autoload 'vitte-mode "vitte-mode" nil t)
(add-to-list 'auto-mode-alist '("\\.vit\\'" . vitte-mode))
```

### Nano
Add to ~/.nanorc:
```
include "~/.config/nano/vitte.nanorc"
```

## Common Commands

```bash
# Show help
vitte --help

# Show version
vitte --version

# Compile a program
vitte myprogram.vit

# Run with debug info
vitte --debug myprogram.vit

# Check syntax without compiling
vitte --check myprogram.vit

# Get compiler info
vitte --info
```

## Project Structure

Recommended layout for Vitte projects:

```
myproject/
├── src/
│   ├── main.vit
│   ├── module1.vit
│   └── module2.vit
├── tests/
│   ├── test_module1.vit
│   └── test_module2.vit
├── README.md
└── Makefile
```

## Resources

- **Documentation:** `/usr/local/share/vitte/docs/`
- **Examples:** `/usr/local/share/vitte/tests/modules/`
- **Standard Library:** `/usr/local/share/vitte/src/vitte/packages/`
- **GitHub:** https://github.com/vitte-lang/vitte

## Environment Variables

```bash
# Set compiler root (usually auto-detected)
export VITTE_ROOT="/usr/local/share/vitte"

# Add to PATH
export PATH="/usr/local/bin:$PATH"
```

## Troubleshooting

### Command not found
```bash
# Add to ~/.zshrc or ~/.bash_profile
export PATH="/usr/local/bin:$PATH"
source ~/.zshrc
```

### Compiler errors
```bash
# Get detailed error messages
vitte --verbose myprogram.vit

# Check syntax
vitte --check myprogram.vit
```

### Editor not recognizing .vit files
- Restart your editor
- Check that editor syntax file is installed
- Try: `vitte --info` to verify installation

## Getting Help

- **Online Docs:** https://vitte-lang.github.io/
- **Issue Tracker:** https://github.com/vitte-lang/vitte/issues
- **Discussions:** https://github.com/vitte-lang/vitte/discussions

---

**Next Steps:** Read the full documentation at `/usr/local/share/vitte/docs/`
EOF

  success "Quickstart guide created: QUICKSTART.md"
}

create_checksum_verification() {
  log "Creating checksum verification script..."
  
  cat > "$OUT_DIR/verify.sh" << 'EOF'
#!/usr/bin/env bash
set -euo pipefail

VERSION="${1:-.}"

if [ ! -f "vitte-$VERSION.pkg.sha256" ]; then
  echo "Error: vitte-$VERSION.pkg.sha256 not found"
  exit 1
fi

echo "Verifying vitte-$VERSION.pkg..."

if command -v sha256sum &>/dev/null; then
  sha256sum -c "vitte-$VERSION.pkg.sha256"
else
  shasum -a 256 -c "vitte-$VERSION.pkg.sha256"
fi

echo "✓ Package verification successful!"
EOF
  
  chmod +x "$OUT_DIR/verify.sh"
  success "Verification script created: verify.sh"
}

main() {
  log "========================================================"
  log "Creating Distribution Bundle"
  log "========================================================"
  log "Version: $VERSION"
  log "Output:  $OUT_DIR"
  log "========================================================\n"
  
  mkdir -p "$OUT_DIR"
  
  create_checksums
  create_release_notes
  create_quickstart
  create_checksum_verification
  
  if [ "$CREATE_DMG" = "1" ]; then
    create_dmg || warn "DMG creation skipped"
  fi
  
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
