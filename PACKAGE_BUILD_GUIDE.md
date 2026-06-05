# Vitte macOS Package Build Guide

## Overview

This guide explains how to build a complete Vitte distribution package for macOS x86_64 with all components:

- **Compiler & Runtime** - Fully compiled Vitte compiler
- **Standard Library** - Complete standard library packages
- **Editor Support** - Vim, Emacs, Nano, Geany, Tree-sitter
- **Documentation** - Full API and language documentation
- **Tools & Utilities** - Shell completions, man pages, debugging tools
- **Installer Package** - macOS .pkg installer for easy distribution

## Quick Start

### Build Complete Package (Recommended)

```bash
cd /path/to/vitte
make -f Makefile.pkg pkg-complete
```

This single command:
1. Cleans previous builds
2. Compiles the Vitte compiler for x86_64
3. Creates a universal binary
4. Bundles all components
5. Generates the .pkg installer
6. Creates distribution files (checksums, docs, quickstart)
7. Outputs everything to `pkgout/`

### Alternative: Step-by-Step Build

```bash
# Step 1: Build the .pkg installer
make -f Makefile.pkg pkg

# Step 2: Create distribution bundle
make -f Makefile.pkg pkg-bundle

# Step 3: Verify package integrity
make -f Makefile.pkg verify-pkg

# Step 4: Install (optional)
make -f Makefile.pkg install-pkg
```

## Build Scripts

### 1. build-macos-x86_64-universal-pkg.sh

**Purpose:** Compile Vitte and create the .pkg installer

**What it does:**
- Checks macOS prerequisites (Xcode, build tools)
- Compiles Vitte compiler for x86_64
- Creates universal binary (x86_64 optimized)
- Bundles standard library and packages
- Collects all editor support files
- Generates checksums and signatures
- Creates macOS .pkg installer

**Usage:**
```bash
./scripts/build-macos-x86_64-universal-pkg.sh
```

**Environment Variables:**
```bash
OUT_DIR=pkgout              # Output directory
VERSION=2.1.1              # Package version
JOBS=4                     # Parallel compilation jobs
OPT_LEVEL=2                # Optimization level (0-3)
PREFIX=/usr/local          # Installation prefix
```

### 2. build-distribution-bundle.sh

**Purpose:** Create checksums, documentation, and quickstart guides

**What it does:**
- Generates SHA256 and SHA512 checksums
- Creates installation guide (INSTALL.md)
- Creates quickstart guide (QUICKSTART.md)
- Generates release notes (RELEASE-VERSION.md)
- Creates package manifest
- Generates verification script
- Optionally creates DMG image

**Usage:**
```bash
./scripts/build-distribution-bundle.sh

# With DMG creation
CREATE_DMG=1 ./scripts/build-distribution-bundle.sh
```

### 3. build-complete-pkg.sh

**Purpose:** Orchestrate the complete build workflow

**What it does:**
- Validates all scripts are present
- Runs the compiler build
- Runs the distribution bundle creation
- Displays final summary with all outputs

**Usage:**
```bash
./scripts/build-complete-pkg.sh
```

## Output Structure

After building, your `pkgout/` directory contains:

```
pkgout/
├── vitte-2.1.1.pkg                 # Main installer package
├── vitte-2.1.1.pkg.sha256          # SHA256 checksum
├── vitte-2.1.1.pkg.sha512          # SHA512 checksum
├── INSTALL.md                       # Installation guide
├── QUICKSTART.md                    # Quick start guide
├── RELEASE-2.1.1.md                # Release notes
├── PACKAGE_MANIFEST.txt            # Package contents
└── verify.sh                        # Verification script
```

## System Requirements for Building

### macOS
- macOS 10.13 or later
- x86_64 processor
- Xcode Command Line Tools (`xcode-select --install`)

### Disk Space
- 500 MB for build directory
- 200-300 MB for final package
- 1 GB total recommended

### Build Tools
- make
- gcc/clang (from Xcode)
- rsync
- pkgbuild/productbuild (from Xcode)

## Building on macOS

### Step 1: Install Xcode Command Line Tools

```bash
xcode-select --install
```

### Step 2: Clone or navigate to Vitte repository

```bash
cd /path/to/vitte
```

### Step 3: Run the build

```bash
# Complete build (recommended)
make -f Makefile.pkg pkg-complete

# Or run script directly
./scripts/build-complete-pkg.sh
```

### Step 4: Find your package

```bash
ls -lh pkgout/
```

## Customization

### Change Output Directory

```bash
OUT_DIR=/path/to/output make -f Makefile.pkg pkg-complete
```

### Change Number of Parallel Jobs

```bash
JOBS=8 make -f Makefile.pkg pkg
```

### Change Optimization Level

```bash
OPT_LEVEL=3 make -f Makefile.pkg pkg  # Maximum optimization
OPT_LEVEL=0 make -f Makefile.pkg pkg  # Debug build
```

### Create DMG Image

```bash
CREATE_DMG=1 make -f Makefile.pkg pkg-bundle
```

## Verification

### Verify Package Integrity

```bash
cd pkgout
bash verify.sh vitte-2.1.1
```

### Manual Checksum Verification

```bash
cd pkgout
shasum -a 256 -c vitte-2.1.1.pkg.sha256
```

## Installation

### For End Users

```bash
# Method 1: Double-click the .pkg file in Finder
open vitte-2.1.1.pkg

# Method 2: Command line
sudo installer -pkg vitte-2.1.1.pkg -target /
```

### Post-Installation

```bash
# Verify installation
vitte --version

# Check location
which vitte
/usr/local/bin/vitte

# View documentation
open /usr/local/share/vitte/docs/
```

## Package Contents

### Installation Paths

After installation:

```
/usr/local/
├── bin/
│   ├── vitte       (main compiler)
│   └── vittec      (compiler wrapper)
│
└── share/vitte/
    ├── src/
    │   ├── vitte/compiler/      (compiler source)
    │   ├── vitte/packages/      (standard library)
    │   └── compiler/backends/   (backend sources)
    │
    ├── editors/                 (editor support)
    ├── docs/                    (full documentation)
    ├── completions/             (shell completions)
    ├── tests/modules/           (test modules)
    │
    ├── LICENSE                  (license file)
    ├── CHANGELOG                (changelog)
    ├── VERSION                  (version file)
    │
    ├── env.sh                   (environment setup)
    ├── uninstall.sh             (uninstaller script)
    │
    ├── checksums/               (verification files)
    │   ├── vitte.sha256
    │   ├── vitte.sha512
    │   └── vitte.signature.txt
    │
    └── manifest/                (package metadata)
```

## Troubleshooting

### Build fails on "xcode-select"
```bash
# Install Xcode Command Line Tools
xcode-select --install
```

### "Command not found" during build
```bash
# Make sure tools are installed
brew install make rsync

# Or use Xcode tools
sudo xcode-select --switch /Applications/Xcode.app/Contents/Developer
```

### Package file not created
```bash
# Check if previous build succeeded
ls -la bin/vitte

# Run with verbose output
bash -x scripts/build-macos-x86_64-universal-pkg.sh
```

### Permissions error on install
```bash
# Use sudo for system-wide installation
sudo installer -pkg vitte-2.1.1.pkg -target /

# Or install to user directory (create custom script for this)
```

## Distribution

### GitHub Releases

1. Build the package:
   ```bash
   make -f Makefile.pkg pkg-complete
   ```

2. Create GitHub release with:
   - `vitte-*.pkg` (installer)
   - `vitte-*.pkg.sha256` (checksum)
   - `RELEASE-*.md` (release notes)
   - `QUICKSTART.md` (quick start)

### Package Managers

The .pkg can be integrated with:
- [Homebrew](https://brew.sh/) (custom cask)
- [MacPorts](https://www.macports.org/)
- Direct distribution

### Docker Image

To distribute via container:
```dockerfile
FROM macOS:latest
COPY pkgout/vitte-*.pkg /tmp/
RUN installer -pkg /tmp/vitte-*.pkg -target /
```

## Environment Setup

### Shell Profile

Add to `~/.zshrc` or `~/.bash_profile`:

```bash
# Vitte compiler path
export PATH="/usr/local/bin:$PATH"

# Vitte runtime root
export VITTE_ROOT="/usr/local/share/vitte"

# Shell completions (bash)
source /usr/local/share/vitte/completions/vitte.bash

# Shell completions (zsh)
source /usr/local/share/vitte/completions/vitte.zsh
```

## Advanced Configuration

### Custom Installation Prefix

Modify `Makefile` before building:
```make
PREFIX = /opt/vitte
```

Or use environment variable:
```bash
PREFIX=/opt/vitte make -f Makefile.pkg pkg
```

### Build with PGO (Profile-Guided Optimization)

```bash
PGO_MODE=instrument make -f Makefile.pkg pkg
# Run some workloads...
PGO_MODE=optimize make -f Makefile.pkg pkg
```

### Build with LTO (Link-Time Optimization)

```bash
LTO=1 make -f Makefile.pkg pkg
```

## Support & Documentation

- **Full Documentation:** `/usr/local/share/vitte/docs/`
- **Quickstart Guide:** Run `cat pkgout/QUICKSTART.md`
- **GitHub:** https://github.com/vitte-lang/vitte
- **Issues:** https://github.com/vitte-lang/vitte/issues

## License

Vitte is licensed under the MIT License. See `/usr/local/share/vitte/LICENSE` for details.

---

**Last Updated:** 2024-06-05
**Vitte Version:** 2.1.1+
