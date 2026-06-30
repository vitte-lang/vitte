# Vitte macOS x86_64 Package Build Infrastructure

## Summary

This is a complete build and packaging infrastructure for creating Vitte language distribution packages for macOS x86_64.

## What's Included

### Build Scripts
- **`scripts/build-macos-x86_64-universal-pkg.sh`** - Main compiler build and .pkg creation
- **`scripts/build-distribution-bundle.sh`** - Distribution files and documentation
- **`scripts/build-complete-pkg.sh`** - Orchestrator for complete workflow

### Build Configuration
- **`Makefile.pkg`** - GNU Make targets for building and installation
- **`PACKAGE_BUILD_GUIDE.md`** - Comprehensive building guide

### Output
- **`pkgout/`** - Generated packages, checksums, and documentation

## Quick Start

### Build Everything (Recommended)

```bash
cd /path/to/vitte
make -f Makefile.pkg pkg-complete
```

This creates a complete distribution package with:
- ✓ Compiled Vitte compiler (x86_64)
- ✓ Standard library
- ✓ Editor support (Vim, Emacs, Nano, Geany, Tree-sitter)
- ✓ Documentation & completions
- ✓ macOS .pkg installer
- ✓ Checksums & signatures
- ✓ Installation guides

### Output

```
pkgout/
├── vitte-2.1.1.pkg              # macOS installer package
├── vitte-2.1.1.pkg.sha256       # SHA256 checksum
├── vitte-2.1.1.pkg.sha512       # SHA512 checksum
├── INSTALL.md                   # Installation guide
├── QUICKSTART.md                # Quick start tutorial
├── RELEASE-2.1.1.md             # Release notes
├── PACKAGE_MANIFEST.txt         # Package contents
└── verify.sh                     # Verification script
```

## Build Targets

```bash
# Complete workflow (recommended)
make -f Makefile.pkg pkg-complete

# Build steps individually:
make -f Makefile.pkg pkg           # Build .pkg installer
make -f Makefile.pkg pkg-bundle    # Create distribution bundle
make -f Makefile.pkg verify-pkg    # Verify package integrity
make -f Makefile.pkg install-pkg   # Install the package

# Utilities
make -f Makefile.pkg pkg-clean     # Clean build artifacts
make -f Makefile.pkg help          # Show help
```

## System Requirements

### For Building
- macOS 10.13 or later
- x86_64 processor
- Xcode Command Line Tools
- 1 GB disk space

### For Installation (End Users)
- macOS 10.13 or later
- x86_64 processor
- 500 MB free disk space

## Installation Guide

### For End Users

1. Download `vitte-2.1.1.pkg`
2. Double-click to open installer OR use:
   ```bash
   sudo installer -pkg vitte-2.1.1.pkg -target /
   ```
3. Verify: `vitte --version`

### For Developers (Building from Source)

See `PACKAGE_BUILD_GUIDE.md` for detailed instructions.

## Directory Structure

```
vitte/
├── scripts/
│   ├── build-macos-x86_64-universal-pkg.sh    # Compiler build script
│   ├── build-distribution-bundle.sh            # Distribution creation
│   └── build-complete-pkg.sh                   # Orchestrator
│
├── Makefile.pkg                                # Build targets
├── PACKAGE_BUILD_GUIDE.md                      # Building guide
├── PACKAGE_BUILD_README.md                     # This file
│
└── pkgout/                                     # Generated packages
    ├── vitte-2.1.1.pkg
    ├── vitte-2.1.1.pkg.sha256
    ├── INSTALL.md
    ├── QUICKSTART.md
    └── ...
```

## Package Contents

The created package includes:

### Binaries
- `/usr/local/bin/vitte` - Main compiler
- `/usr/local/bin/vittec` - Compiler wrapper
- `/usr/local/libexec/vitte/` - Internal tools

### Standard Library
- `/usr/local/share/vitte/src/vitte/packages/` - Standard library
- `/usr/local/share/vitte/src/vitte/compiler/` - Compiler source
- Complete type system and runtime support

### Editor Support
- **Vim** - Syntax, indentation, file detection
- **Emacs** - Full IDE mode with completions
- **Nano** - Lightweight syntax highlighting
- **Geany** - GTK+ IDE integration
- **Tree-sitter** - Modern AST grammar

### Documentation
- Full API documentation
- Language specification
- Examples and tutorials
- Shell completions (bash, zsh)
- Man pages

### Tools
- Installation/uninstallation scripts
- Environment setup helpers
- Verification tools
- Diagnostic utilities

## Customization

### Change Output Location

```bash
OUT_DIR=/custom/path make -f Makefile.pkg pkg-complete
```

### Optimize Build

```bash
JOBS=8 OPT_LEVEL=3 make -f Makefile.pkg pkg
```

### Create DMG Image

```bash
CREATE_DMG=1 make -f Makefile.pkg pkg-bundle
```

## Development Workflow

### For Package Maintainers

```bash
# 1. Build new version
make -f Makefile.pkg pkg-complete

# 2. Verify integrity
cd pkgout && bash verify.sh vitte-2.1.1

# 3. Sign the package (optional)
codesign -s - pkgout/vitte-2.1.1.pkg

# 4. Distribute
# Upload to GitHub releases, package repositories, etc.
```

### For Contributors

```bash
# Test changes to build scripts
./scripts/build-macos-x86_64-universal-pkg.sh

# Add new components
# Edit scripts to include them in the build

# Verify changes
make -f Makefile.pkg pkg-complete
```

## Verification

### Verify Package Integrity

```bash
cd pkgout
bash verify.sh vitte-2.1.1
# Output: ✓ Package verification successful!
```

### Manual Verification

```bash
# SHA256
shasum -a 256 -c pkgout/vitte-2.1.1.pkg.sha256

# SHA512  
shasum -a 512 -c pkgout/vitte-2.1.1.pkg.sha512
```

## Post-Installation

### Verify Installation

```bash
# Check binary is accessible
which vitte
# Output: /usr/local/bin/vitte

# Check version
vitte --version
# Output: Vitte 2.1.1

# Try compilation
echo 'fn main() { println("Hello, Vitte!") }' > test.vit
vitte test.vit && ./test
# Output: Hello, Vitte!
```

### Editor Setup

See `/usr/local/share/vitte/docs/` for:
- Vim configuration
- Emacs setup
- Nano integration
- Geany integration

## Troubleshooting

### Build Fails: "xcode-select --install"

```bash
# Install Xcode Command Line Tools
xcode-select --install
```

### Build Fails: "rsync not found"

```bash
# Install rsync via Homebrew
brew install rsync
```

### Package Creation Fails

```bash
# Ensure previous build is clean
make -f Makefile.pkg pkg-clean

# Run with verbose output
bash -x scripts/build-macos-x86_64-universal-pkg.sh
```

### Installation Fails: Permission Denied

```bash
# Use sudo for system-wide installation
sudo installer -pkg pkgout/vitte-2.1.1.pkg -target /
```

## Distribution

### GitHub Releases

```bash
# 1. Build package
make -f Makefile.pkg pkg-complete

# 2. Create release with:
# - vitte-2.1.1.pkg
# - vitte-2.1.1.pkg.sha256
# - RELEASE-2.1.1.md
# - QUICKSTART.md
```

### Package Managers

The .pkg can be integrated with:
- **Homebrew** (via custom cask)
- **MacPorts** (via port file)
- Direct web distribution

## Documentation

- **Building Guide:** `PACKAGE_BUILD_GUIDE.md`
- **Installation Guide:** `pkgout/INSTALL.md`
- **Quick Start:** `pkgout/QUICKSTART.md`
- **Release Notes:** `pkgout/RELEASE-2.1.1.md`
- **Main Docs:** `/usr/local/share/vitte/docs/` (after installation)

## Support

- **GitHub:** https://github.com/vitte-lang/vitte
- **Issues:** https://github.com/vitte-lang/vitte/issues
- **Documentation:** https://vitte-lang.github.io/

## Files Created

This infrastructure creates the following files:

### Scripts
```bash
scripts/build-macos-x86_64-universal-pkg.sh   (10.6 KB)
scripts/build-distribution-bundle.sh          (10.3 KB)
scripts/build-complete-pkg.sh                 (5.6 KB)
```

### Configuration
```bash
Makefile.pkg                                  (4.0 KB)
PACKAGE_BUILD_GUIDE.md                        (8.7 KB)
PACKAGE_BUILD_README.md                       (this file)
```

### Generated Output (in pkgout/)
```
vitte-2.1.1.pkg                              (~150-200 MB)
vitte-2.1.1.pkg.sha256                       (100 bytes)
vitte-2.1.1.pkg.sha512                       (200 bytes)
INSTALL.md                                    (~8 KB)
QUICKSTART.md                                 (~6 KB)
RELEASE-2.1.1.md                             (~5 KB)
PACKAGE_MANIFEST.txt                         (~3 KB)
verify.sh                                     (~1 KB)
```

## License

Vitte is licensed under the MIT License. All packaging infrastructure is also MIT licensed.

## Authors

- Original Vitte language and compiler - Vitte contributors
- macOS packaging infrastructure - Created as part of Vitte distribution

---

**Ready to build?** Run: `make -f Makefile.pkg pkg-complete`

For detailed information, see `PACKAGE_BUILD_GUIDE.md`
