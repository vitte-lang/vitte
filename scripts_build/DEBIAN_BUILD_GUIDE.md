# Vitte Debian x86_64 Package Build Guide

## Overview

This guide explains how to build a complete Vitte distribution package for Debian/Ubuntu x86_64 with all components:

- **Compiler & Runtime** - Fully compiled Vitte compiler
- **Standard Library** - Complete standard library packages
- **Editor Support** - Vim, Emacs, Nano, Geany, Tree-sitter
- **Documentation** - Full API and language documentation
- **Tools & Utilities** - Shell completions, debugging tools
- **Installer Package** - Debian .deb installer for easy distribution

## Quick Start

### Build Complete Package (Recommended)

```bash
cd /path/to/vitte
make -f Makefile.deb deb-complete
```

This single command:
1. Cleans previous builds
2. Compiles the Vitte compiler for x86_64
3. Bundles all components
4. Generates the .deb package
5. Creates distribution files (checksums, docs, quickstart)
6. Outputs everything to `pkgout/`

### Alternative: Step-by-Step Build

```bash
# Step 1: Build the .deb package
make -f Makefile.deb deb

# Step 2: Create distribution bundle
make -f Makefile.deb deb-bundle

# Step 3: Verify package integrity
make -f Makefile.deb verify-deb

# Step 4: Install (optional)
make -f Makefile.deb install-deb
```

## Build Scripts

### 1. build-debian-x86_64-deb.sh

**Purpose:** Compile Vitte and create the .deb package

**What it does:**
- Checks Linux and Debian prerequisites
- Compiles Vitte compiler for x86_64
- Bundles standard library and packages
- Collects all editor support files
- Generates Debian control files
- Creates postinst/prerm scripts
- Creates .deb package

**Usage:**
```bash
./scripts/build-debian-x86_64-deb.sh
```

**Environment Variables:**
```bash
OUT_DIR=pkgout              # Output directory
VERSION=2.1.1              # Package version
ARCH=amd64                 # Architecture
JOBS=4                     # Parallel compilation jobs
OPT_LEVEL=2                # Optimization level (0-3)
PREFIX=/usr                # Installation prefix
```

### 2. build-debian-distribution-bundle.sh

**Purpose:** Create checksums, documentation, and quickstart guides

**What it does:**
- Generates SHA256 and SHA512 checksums
- Creates installation guide (INSTALL-DEBIAN.md)
- Creates quickstart guide (QUICKSTART-DEBIAN.md)
- Generates release notes
- Creates package manifest
- Generates verification script

**Usage:**
```bash
./scripts/build-debian-distribution-bundle.sh
```

### 3. build-debian-complete-pkg.sh

**Purpose:** Orchestrate the complete build workflow

**What it does:**
- Validates all scripts are present
- Runs the compiler build
- Runs the distribution bundle creation
- Displays final summary with all outputs

**Usage:**
```bash
./scripts/build-debian-complete-pkg.sh
```

## Output Structure

After building, your `pkgout/` directory contains:

```
pkgout/
├── vitte_2.1.1_amd64.deb               # Main installer package
├── vitte_2.1.1_amd64.deb.sha256        # SHA256 checksum
├── vitte_2.1.1_amd64.deb.sha512        # SHA512 checksum
├── INSTALL-DEBIAN.md                   # Installation guide
├── QUICKSTART-DEBIAN.md                # Quick start guide
├── RELEASE-DEBIAN-2.1.1.md             # Release notes
├── PACKAGE_MANIFEST-DEBIAN.txt         # Package contents
└── verify-debian.sh                    # Verification script
```

## System Requirements for Building

### Linux Distribution
- Debian 10 (Buster) or later
- Ubuntu 18.04 or later
- Any Debian-based distribution with dpkg

### Build Tools
- make
- dpkg-deb (usually pre-installed)
- gcc/g++ (from build-essential)
- rsync
- python3

### Disk Space
- 500 MB for build directory
- 150-250 MB for final package
- 1 GB total recommended

## Building on Debian/Ubuntu

### Step 1: Install Build Dependencies

```bash
# On Debian/Ubuntu
sudo apt update
sudo apt install build-essential dpkg-dev rsync python3 git

# Optional but recommended
sudo apt install gcc g++ make pkg-config
```

### Step 2: Clone or Navigate to Vitte Repository

```bash
cd /path/to/vitte
```

### Step 3: Run the Build

```bash
# Complete build (recommended)
make -f Makefile.deb deb-complete

# Or run script directly
./scripts/build-debian-complete-pkg.sh
```

### Step 4: Find Your Package

```bash
ls -lh pkgout/*.deb
```

## Customization

### Change Output Directory

```bash
OUT_DIR=/path/to/output make -f Makefile.deb deb-complete
```

### Change Number of Parallel Jobs

```bash
JOBS=8 make -f Makefile.deb deb
```

### Change Optimization Level

```bash
OPT_LEVEL=3 make -f Makefile.deb deb  # Maximum optimization
OPT_LEVEL=0 make -f Makefile.deb deb  # Debug build
```

### Build for Different Architecture

```bash
# For i386 (32-bit)
ARCH=i386 make -f Makefile.deb deb

# For arm64
ARCH=arm64 make -f Makefile.deb deb

# For armhf
ARCH=armhf make -f Makefile.deb deb
```

## Verification

### Verify Package Integrity

```bash
cd pkgout
bash verify-debian.sh amd64 vitte 2.1.1
```

### Manual Checksum Verification

```bash
cd pkgout
sha256sum -c vitte_2.1.1_amd64.deb.sha256
```

### Verify Package Contents

```bash
dpkg-deb -c pkgout/vitte_2.1.1_amd64.deb | head -20
```

## Installation

### For End Users

```bash
# Method 1: Using apt
sudo apt install ./vitte_2.1.1_amd64.deb

# Method 2: Using dpkg
sudo dpkg -i vitte_2.1.1_amd64.deb
sudo apt install -f  # Install dependencies if needed

# Method 3: From URL
sudo apt install https://example.com/vitte_2.1.1_amd64.deb
```

### Post-Installation

```bash
# Verify installation
vitte --version

# Check location
which vitte
/usr/bin/vitte

# View documentation
ls /usr/share/vitte/docs/
```

## Package Contents

### Installation Paths

After installation:

```
/usr/
├── bin/
│   └── vitte       (main compiler)
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
    ├── LICENSE                  (license file)
    ├── CHANGELOG                (changelog)
    ├── VERSION                  (version file)
    └── ...
```

## Troubleshooting

### Build fails on "dpkg-deb not found"

```bash
# Install dpkg-dev
sudo apt install dpkg-dev
```

### "gcc: command not found"

```bash
# Install build-essential
sudo apt install build-essential
```

### "rsync not found"

```bash
# Install rsync
sudo apt install rsync
```

### Package file not created

```bash
# Check if previous build succeeded
ls -la bin/vitte

# Run with verbose output
bash -x scripts/build-debian-x86_64-deb.sh
```

### Installation fails with dependency errors

```bash
# Install missing dependencies
sudo apt install -f

# Or manually install
sudo apt install bash libc6
```

## Creating a PPA (Personal Package Archive)

To distribute your package via PPA:

1. Build the package:
   ```bash
   make -f Makefile.deb deb-complete
   ```

2. Create source package (if needed):
   ```bash
   dpkg-source -b .
   ```

3. Upload to Launchpad PPA:
   ```bash
   dput ppa:yourname/vitte pkgout/vitte_*.deb
   ```

## Distribution

### GitHub Releases

1. Build the package:
   ```bash
   make -f Makefile.deb deb-complete
   ```

2. Create GitHub release with:
   - `vitte_*.deb` (installer)
   - `vitte_*.deb.sha256` (checksum)
   - `RELEASE-DEBIAN-*.md` (release notes)
   - `INSTALL-DEBIAN.md` (installation guide)

### Package Repository

The .deb can be added to:
- Debian repositories
- Ubuntu PPAs
- Linux Mint repositories
- Elementary OS repositories
- Custom apt repositories

## Environment Setup

### Shell Profile

Add to `~/.bashrc` or `~/.zshrc`:

```bash
# Vitte compiler path
export PATH="/usr/bin:$PATH"

# Vitte runtime root
export VITTE_ROOT="/usr/share/vitte"

# Shell completions (bash)
source /usr/share/vitte/completions/vitte.bash

# Shell completions (zsh)
source /usr/share/vitte/completions/vitte.zsh
```

## Support & Documentation

- **Full Documentation:** `/usr/share/vitte/docs/`
- **Quickstart Guide:** Run `cat pkgout/QUICKSTART-DEBIAN.md`
- **GitHub:** https://github.com/vitte-lang/vitte
- **Issues:** https://github.com/vitte-lang/vitte/issues

## License

Vitte is licensed under the MIT License. See `/usr/share/vitte/LICENSE` for details.

---

**Last Updated:** 2024-06-05
**Vitte Version:** 2.1.1+
**Platform:** Debian/Ubuntu x86_64
