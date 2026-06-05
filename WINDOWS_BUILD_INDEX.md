# Vitte Windows x86_64 Package Build - Quick Start Guide

## 📖 Overview

This guide covers building **Windows x86_64 (64-bit)** installers for Vitte.

### 🎯 Use This If You Have:
- **Windows x86_64 system** (64-bit Intel/AMD)
- **Linux system with build tools** (to create installer)
- **Need to distribute to Windows users**

---

## 🚀 Quick Start

### Build Windows x86_64 Installer (5-15 minutes)

```bash
cd /path/to/vitte
make -f Makefile.exe exe-complete
```

This automatically:
- ✅ Compiles Vitte for x86_64 (64-bit)
- ✅ Bundles standard library
- ✅ Includes editor support
- ✅ Creates Windows .exe installer (NSIS)
- ✅ Generates checksums
- ✅ Creates documentation

**Output:** `pkgout/vitte-2.1.1-x86_64-installer.exe`

---

## 📋 What Was Created

### 🔧 Build Scripts (in `scripts/windows/`)

| Script | Purpose |
|--------|---------|
| `build-windows-x86_64-exe.sh` | Compiler build & NSIS packaging |
| `build-windows-distribution-bundle.sh` | Checksums & documentation |

### ⚙️ Build Configuration

| File | Purpose |
|------|---------|
| `Makefile.exe` | GNU Make targets for Windows builds |
| `toolchain/scripts/package/windows/vitte-installer.nsi` | NSIS installer script |

---

## 🎯 Common Tasks

### Build the Installer
```bash
make -f Makefile.exe exe-complete
```

### View Build Options
```bash
make -f Makefile.exe help
```

### Verify Installer
```bash
cd pkgout && sha256sum -c vitte-2.1.1-x86_64-installer.exe.sha256
```

### Install on Windows
1. Download: `vitte-2.1.1-x86_64-installer.exe`
2. Double-click to run installer
3. Follow installation wizard
4. Choose installation directory (default: `C:\Program Files\Vitte`)
5. Select components
6. Click Install

---

## 📁 Output Structure

After building:

```
pkgout/
├── vitte-2.1.1-x86_64-installer.exe          (.exe installer)
├── vitte-2.1.1-x86_64-installer.exe.sha256   (checksum)
├── vitte-2.1.1-x86_64-installer.exe.sha512   (checksum)
├── INSTALL-WINDOWS-X86_64.md                 (installation guide)
└── RELEASE-WINDOWS-X86_64-2.1.1.md           (release notes)
```

---

## ✅ Requirements

### On Linux (to build Windows installer):
- `makensis` (NSIS) → `sudo apt install nsis`
- `gcc` or `mingw-w64` → `gcc` or `sudo apt install mingw-w64`
- `make` → already installed
- 1 GB disk space

### On Windows (to run installer):
- Windows 7 SP1 or later
- x86_64 processor (64-bit)
- ~500 MB disk space
- Administrator privileges

---

## 💡 Customization

### More Parallel Jobs
```bash
JOBS=8 make -f Makefile.exe exe-complete
```

### Maximum Optimization
```bash
OPT_LEVEL=3 make -f Makefile.exe exe
```

### Custom Output
```bash
OUT_DIR=~/releases make -f Makefile.exe exe-complete
```

---

## 🌍 Supported Windows Versions

- ✓ Windows 7 SP1+
- ✓ Windows 8/8.1
- ✓ Windows 10 (all versions)
- ✓ Windows 11
- ✓ Server 2008 R2+

---

## 🔗 Links

- [Detailed Build Guide](WINDOWS_BUILD_GUIDE.md)
- [Platform Comparison](PLATFORM_BUILD_GUIDE.md)

---

_Created: 2024-06-05 | Vitte 2.1.1+ | Windows x86_64_
