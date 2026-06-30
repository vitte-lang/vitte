# Vitte Debian i586 (32-bit Pentium) Package Build Guide

**Version:** 2.1.1  
**Platform:** Debian/Ubuntu i586 (32-bit Pentium)  
**Last Updated:** 2024-06-05  
**Audience:** Developers building for Pentium systems

---

## Table of Contents

1. [Overview](#overview)
2. [System Requirements](#system-requirements)
3. [Prerequisites Setup](#prerequisites-setup)
4. [Build Steps](#build-steps)
5. [Customization](#customization)
6. [Troubleshooting](#troubleshooting)

---

## Overview

This guide explains how to build Vitte packages for **32-bit Pentium (i586) systems**.

### Key Features (i586 Pentium)

- **Architecture:** i586 (32-bit x86 Pentium)
- **Output:** `.deb` package for i386 architecture
- **Compiler:** GCC with `-march=i586 -mtune=pentium` flags
- **Target Audience:** Pentium, Pentium MMX systems

### When to Use i586 Build

✓ You have a Pentium processor (original or MMX)  
✓ You need to support old Pentium systems  
✓ You're building for legacy embedded devices  
✓ You need maximum compatibility with Pentium era hardware  

✗ Use i686 instead: Pentium Pro/II/III/IV → Use `DEBIAN_BUILD_GUIDE_I686.md`

---

## System Requirements

### Processor
- Intel Pentium (original)
- Intel Pentium MMX
- Compatible i586 processor
- **Note:** No Pentium Pro+ features used (i686 extensions disabled)

### Operating System
- Debian 10+ (32-bit Pentium version)
- Ubuntu 18.04+ (32-bit Pentium version)
- Other Debian-based 32-bit distros

### Disk Space
- 1 GB minimum for build
- 500 MB for final .deb package

### Memory
- 512 MB RAM minimum
- 1+ GB recommended

---

## Prerequisites Setup

### 1. Install Build Tools

```bash
sudo apt update
sudo apt install build-essential gcc-multilib g++-multilib
```

### 2. Verify 32-bit Compiler (i586)

```bash
gcc -m32 -v
```

Should work (32-bit support present)

### 3. Install Debian Package Tools

```bash
sudo apt install dpkg-deb
```

---

## Build Steps

### Step 1: Navigate to Repository

```bash
cd /path/to/vitte
```

### Step 2: Full Build

```bash
make -f Makefile.deb-i586 deb-i586-complete
```

**Estimated time:** 5-15 minutes

### Step 3: Verify

```bash
cd pkgout && bash verify-debian-i586.sh i386 vitte 2.1.1
```

---

## Customization

### Parallel Jobs

```bash
JOBS=4 make -f Makefile.deb-i586 deb-i586-complete
```

### Optimization Level

```bash
OPT_LEVEL=0  # Fast build
OPT_LEVEL=3  # Maximum optimization (slower build)
```

### Custom Output

```bash
OUT_DIR=~/releases make -f Makefile.deb-i586 deb-i586-complete
```

---

## i586-Specific Compilation

### Compiler Flags Used

```bash
CFLAGS="-m32 -march=i586 -mtune=pentium -O2 -fPIC"
```

**Explanation:**
- `-m32` - Generate 32-bit code
- `-march=i586` - Target i586 Pentium architecture
- `-mtune=pentium` - Optimize for Pentium CPU
- `-O2` - Standard optimization (configurable)
- `-fPIC` - Position independent code

### What This Means

- ✓ Code runs on Pentium (original)
- ✓ Code runs on Pentium MMX
- ✓ **No** i686 instructions used
- ✓ No SSE, SSE2 optimizations
- ✓ Maximum Pentium compatibility

---

## Installation

### Install Package

```bash
sudo dpkg -i pkgout/vitte_2.1.1_i386.deb
```

### Verify Installation

```bash
vitte --version
which vitte
```

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| `gcc -m32 not found` | `sudo apt install gcc-multilib` |
| `libc6:i386 not found` | `sudo apt install libc6:i386` |
| Build failed | `make -f Makefile.deb-i586 deb-i586-clean` then retry |

---

## Architecture Details

### i586 vs i686

| Feature | i586 (Pentium) | i686 (Pentium Pro+) |
|---------|----------------|-------------------|
| **Makefile** | Makefile.deb-i586 | Makefile.deb-i686 |
| **Flags** | -march=i586 | -march=i686 |
| **Optimization** | -mtune=pentium | -mtune=generic |
| **CPUs** | Pentium, Pentium MMX | Pentium Pro, II, III, IV |
| **Use Case** | Old Pentium | Modern Pentium+ |

---

_Vitte 2.1.1+ | Debian i586 Build Guide | 32-bit Pentium Support_
