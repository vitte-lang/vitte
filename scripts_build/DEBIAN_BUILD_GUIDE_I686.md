# Vitte Debian i686 (32-bit) Package Build Guide

**Version:** 2.1.1  
**Platform:** Debian/Ubuntu i686 (32-bit)  
**Last Updated:** 2024-06-05  
**Audience:** Developers building for 32-bit systems

---

## Table of Contents

1. [Overview](#overview)
2. [System Requirements](#system-requirements)
3. [Prerequisites Setup](#prerequisites-setup)
4. [Build Steps](#build-steps)
5. [Customization](#customization)
6. [Troubleshooting](#troubleshooting)
7. [Verification](#verification)

---

## Overview

This guide explains how to build Vitte packages for **32-bit Debian/Ubuntu systems (i686)**.

### Key Features (32-bit)

- **Architecture:** i686 (32-bit x86)
- **Output:** `.deb` package for i386 architecture
- **Compiler:** GCC with `-m32` flag (cross/native 32-bit)
- **Runtime:** 32-bit libc6
- **Target Audience:** Legacy 32-bit systems

### When to Use 32-bit Build

✓ You have a 32-bit Debian/Ubuntu system  
✓ You need to support legacy 32-bit machines  
✓ You're building for IoT or embedded 32-bit devices  
✓ You need maximum compatibility with older systems  

✗ Use 64-bit instead: Modern systems → Use `DEBIAN_BUILD_GUIDE.md`

---

## System Requirements

### Processor
- Intel/AMD i686 or compatible processor
- Must support 32-bit instruction set

### Operating System
- Debian 10+ (32-bit)
- Ubuntu 18.04+ (32-bit)
- Other Debian-based 32-bit distros

### Disk Space
- 1 GB minimum for build
- 500 MB for final .deb package

### Memory
- 2 GB RAM minimum
- 4+ GB recommended for faster builds

### Network
- Internet access (for downloading deps if needed)

---

## Prerequisites Setup

### 1. Install Build Tools (32-bit)

**Essential packages:**
```bash
sudo apt update
sudo apt install build-essential gcc-multilib g++-multilib
```

**Explanation:**
- `build-essential` - C/C++ compiler, make, binutils
- `gcc-multilib` - 32-bit GCC support
- `g++-multilib` - 32-bit G++ support

### 2. Verify 32-bit Compiler (i686)

```bash
gcc -m32 -v
```

**Expected output:**
```
Using built-in specs.
COLLECT_GCC=gcc
COLLECT_LTO_WRAPPER=...
...
--with-arch=i686
...
```

**If you see "fatal: sorry, unrecognized command-line option '-m32'":**
```bash
sudo apt install gcc-multilib g++-multilib
# Then try again
gcc -m32 -v
```

### 3. Install Debian Package Tools

```bash
sudo apt install dpkg-deb dh-make
```

### 4. (Optional) Install Make

```bash
sudo apt install make
```

---

## Build Steps

### Step 1: Navigate to Repository

```bash
cd /path/to/vitte
```

### Step 2: View Available Options

```bash
make -f Makefile.deb-i686 help
```

**Output shows:**
- Available targets
- Environment variables
- Example commands

### Step 3: Full Build (Recommended)

```bash
make -f Makefile.deb-i686 deb-i686-complete
```

This single command:
1. ✓ Cleans previous artifacts
2. ✓ Compiles Vitte for i686 (32-bit)
3. ✓ Bundles stdlib and editors
4. ✓ Creates .deb installer
5. ✓ Generates checksums
6. ✓ Creates distribution files

**Estimated time:** 5-15 minutes

### Step 4: Check Output

```bash
ls -lh pkgout/
```

**Expected files:**
```
vitte_2.1.1_i386.deb           (main package, 32-bit)
vitte_2.1.1_i386.deb.sha256    (checksum)
vitte_2.1.1_i386.deb.sha512    (checksum)
INSTALL-DEBIAN-I686.md         (installation guide)
RELEASE-DEBIAN-I686-2.1.1.md   (release notes)
verify-debian-i686.sh          (verification script)
```

### Step 5: Verify Package (Optional)

```bash
cd pkgout
bash verify-debian-i686.sh i386 vitte 2.1.1
```

**Expected output:**
```
✓ SHA256 verification passed
✓ SHA512 verification passed
✓ Package integrity verified
```

---

## Customization

### Adjust Number of Parallel Jobs

For faster builds on multi-core systems:

```bash
JOBS=4 make -f Makefile.deb-i686 deb-i686-complete
JOBS=8 make -f Makefile.deb-i686 deb-i686-complete
JOBS=16 make -f Makefile.deb-i686 deb-i686-complete
```

**Recommendation:** `JOBS=$(nproc)` for all cores

### Optimization Level

Control CPU vs. build time:

```bash
OPT_LEVEL=0 make -f Makefile.deb-i686 deb-i686     # Fast build, slow runtime
OPT_LEVEL=1 make -f Makefile.deb-i686 deb-i686     # Balanced
OPT_LEVEL=2 make -f Makefile.deb-i686 deb-i686     # Optimized (default)
OPT_LEVEL=3 make -f Makefile.deb-i686 deb-i686     # Maximum optimization
```

**Default:** `OPT_LEVEL=2`

### Custom Output Directory

```bash
OUT_DIR=~/releases make -f Makefile.deb-i686 deb-i686-complete
```

**Output goes to:** `~/releases/`

### Combined Customization

```bash
JOBS=8 OPT_LEVEL=3 OUT_DIR=/tmp/vitte-build make -f Makefile.deb-i686 deb-i686-complete
```

---

## Troubleshooting

### Issue: "gcc -m32 not found"

**Solution:**
```bash
sudo apt install gcc-multilib g++-multilib
gcc -m32 -v  # Verify
```

### Issue: "libc6:i386 not found"

**Solution:**
```bash
sudo apt install libc6:i386 libstdc++6:i386
```

### Issue: "Build failed at compilation"

**Solution - Clean and rebuild:**
```bash
make -f Makefile.deb-i686 deb-i686-clean
make -f Makefile.deb-i686 deb-i686-complete
```

**If still fails - check dependencies:**
```bash
file /usr/bin/gcc
# Should show: x86 80386 (32-bit capable)
```

### Issue: "Permission denied" when running scripts

**Solution:**
```bash
chmod +x scripts/build-debian-i686-*.sh
make -f Makefile.deb-i686 deb-i686-complete
```

### Issue: "File i686: cannot open (No such file or directory)"

**Solution:**
```bash
# Ensure build completed successfully
make -f Makefile.deb-i686 deb-i686-clean
JOBS=2 make -f Makefile.deb-i686 deb-i686-complete
```

### Issue: Package too large

**Solution:**
```bash
# Use optimization level 3
OPT_LEVEL=3 make -f Makefile.deb-i686 deb-i686
```

---

## Verification

### Check 32-bit Binary

```bash
file pkgout/vitte-compiled-i686
```

**Expected:**
```
ELF 32-bit LSB executable, Intel 80386, version 1, ...
```

### Run Verification Script

```bash
cd pkgout
bash verify-debian-i686.sh i386 vitte 2.1.1
```

### Manual Checksum Verification

```bash
cd pkgout
sha256sum -c vitte_2.1.1_i386.deb.sha256
sha512sum -c vitte_2.1.1_i386.deb.sha512
```

---

## Installation (32-bit)

### Install Package

```bash
sudo dpkg -i pkgout/vitte_2.1.1_i386.deb
```

### Install Dependencies (if needed)

```bash
sudo apt install -f
```

### Verify Installation

```bash
vitte --version
which vitte
```

### Shell Completions

The installer automatically sets up:
- Bash completions
- Fish completions
- Zsh completions
- Shell aliases

---

## Build Process Details

### Stage 1: Compilation (i686-specific)

```bash
# Flags used for 32-bit:
CFLAGS="-m32 -march=i686 -O2"
CXXFLAGS="-m32 -march=i686 -O2"
LDFLAGS="-m32"
```

**What happens:**
1. Compiler invoked with `-m32` flag
2. Generates 32-bit x86 machine code
3. Links against 32-bit libc6
4. Results in i686 ELF binary

### Stage 2: Packaging (.deb)

```bash
# Architecture set to i386 (Debian's 32-bit)
Architecture: i386
```

**What happens:**
1. Control metadata created
2. Binary executable embedded
3. postinst script sets up alternatives
4. prerm script handles removal
5. dpkg-deb creates final .deb archive

### Stage 3: Distribution

```bash
# Checksums generated
SHA256(vitte_2.1.1_i386.deb)
SHA512(vitte_2.1.1_i386.deb)
```

**What happens:**
1. Multiple hash algorithms run
2. Installation guides generated
3. Release notes created
4. Verification script provided

---

## Advanced Usage

### Build Only (Skip Bundling)

```bash
make -f Makefile.deb-i686 deb-i686
```

### Create Distribution Files Only

```bash
make -f Makefile.deb-i686 deb-i686-bundle
```

### Clean Build Artifacts

```bash
make -f Makefile.deb-i686 deb-i686-clean
```

### Debug Build

```bash
OPT_LEVEL=0 make -f Makefile.deb-i686 deb-i686 -v
```

---

## Environment Variables Reference

| Variable | Default | Purpose |
|----------|---------|---------|
| `JOBS` | auto | Parallel compilation jobs |
| `OPT_LEVEL` | 2 | Optimization (0-3) |
| `OUT_DIR` | pkgout | Output directory |
| `VERSION` | 2.1.1 | Package version |
| `ARCH` | i386 | Debian architecture |
| `PACKAGE_NAME` | vitte | Package name |

---

## Platform Comparison

| Feature | i686 (32-bit) | x86_64 (64-bit) |
|---------|---------------|-----------------|
| Architecture | i386 | amd64 |
| Makefile | Makefile.deb-i686 | Makefile.deb |
| Build Script | build-debian-i686-deb.sh | build-debian-x86_64-deb.sh |
| Binary Size | Smaller | Larger |
| Memory Usage | Lower | Higher |
| Performance | Moderate | Maximum |
| Supported OS | Old/Legacy | Modern |

---

## Support & Resources

### Documentation
- [Quick Start Index](DEBIAN_BUILD_I686_INDEX.md)
- [Platform Comparison](PLATFORM_BUILD_GUIDE.md)
- [64-bit Guide](DEBIAN_BUILD_GUIDE.md)

### Commands
- View options: `make -f Makefile.deb-i686 help`
- Build: `make -f Makefile.deb-i686 deb-i686-complete`
- Verify: `cd pkgout && bash verify-debian-i686.sh i386 vitte 2.1.1`

### Reporting Issues
- GitHub: https://github.com/vitte-lang/vitte/issues
- Include OS version, compiler version, and error output

---

## Next Steps

**✓ First Time?**
```bash
make -f Makefile.deb-i686 deb-i686-complete
cd pkgout && bash verify-debian-i686.sh i386 vitte 2.1.1
```

**✓ Want Custom Build?**
```bash
JOBS=8 OPT_LEVEL=3 OUT_DIR=~/releases make -f Makefile.deb-i686 deb-i686-complete
```

**✓ Need to Distribute?**
- Check checksums: `cd pkgout && cat *.sha256`
- Review guide: `cat pkgout/INSTALL-DEBIAN-I686.md`
- Upload to repository

---

_Vitte 2.1.1+ | Debian i686 Build Guide | 32-bit Support_
