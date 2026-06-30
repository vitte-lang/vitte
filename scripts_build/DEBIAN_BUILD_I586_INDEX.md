# Vitte Debian i586 (32-bit Pentium) Package Build - Index & Getting Started

## 📖 Quick Overview

This is the **i586 Pentium** build infrastructure for Debian/Ubuntu systems.

### 🎯 Use This If You Have:
- **32-bit Pentium processor** (Pentium, Pentium MMX)
- **Old i586-based system**
- **Legacy equipment without i686+ support**
- **Need to distribute to Pentium systems**

### ⚠️ Use Different Version If You Have:
- **i686+ Pentium Pro/II/III/IV** → Use `DEBIAN_BUILD_I686_INDEX.md` (i686)
- **Modern 64-bit system** → Use `DEBIAN_BUILD_INDEX.md` (amd64)
- **macOS system** → Use `PKG_BUILD_INDEX.md`

---

## 🚀 Quick Start

### Build i586 Package (5-15 minutes)

```bash
cd /path/to/vitte
make -f Makefile.deb-i586 deb-i586-complete
```

This automatically:
- ✅ Compiles Vitte for i586 Pentium (32-bit)
- ✅ Bundles standard library
- ✅ Includes editor support
- ✅ Creates Debian .deb installer
- ✅ Generates checksums
- ✅ Creates installation guides

**Output:** `pkgout/vitte_2.1.1_i386.deb` (Pentium i586 version)

---

## 📋 What Was Created

### 🔧 Build Scripts (in `scripts/`)

| Script | Purpose |
|--------|---------|
| `build-debian-i586-deb.sh` | Compiler build & packaging (i586 Pentium) |
| `build-debian-i586-distribution-bundle.sh` | Create checksums & docs (i586) |
| `build-debian-i586-complete-pkg.sh` | Orchestrate complete workflow |

### ⚙️ Build Configuration

| File | Purpose |
|------|---------|
| `Makefile.deb-i586` | GNU Make targets for i586 builds |

---

## 📁 Output Structure

After building:

```
pkgout/
├── vitte_2.1.1_i386.deb              (Pentium i586 installer)
├── vitte_2.1.1_i386.deb.sha256       (checksum)
├── vitte_2.1.1_i386.deb.sha512       (checksum)
├── INSTALL-DEBIAN-I586.md            (i586 guide)
├── RELEASE-DEBIAN-I586-2.1.1.md      (i586 notes)
└── verify-debian-i586.sh             (i586 verification)
```

---

## 🎯 Common Tasks

### Build the Package
```bash
make -f Makefile.deb-i586 deb-i586-complete
```

### View Build Options
```bash
make -f Makefile.deb-i586 help
```

### Verify Package (i586 Pentium)
```bash
cd pkgout && bash verify-debian-i586.sh i386 vitte 2.1.1
```

### Install Package (i586 Pentium)
```bash
sudo dpkg -i pkgout/vitte_2.1.1_i386.deb
sudo apt install -f  # If dependencies needed
```

### Clean Build Artifacts
```bash
make -f Makefile.deb-i586 deb-i586-clean
```

---

## ✅ Checklist

### Before Building
- [ ] Debian 10+ or Ubuntu 18.04+ (32-bit Pentium)
- [ ] i586 processor (Pentium, Pentium MMX)
- [ ] Build tools: `sudo apt install build-essential gcc-multilib`
- [ ] 1 GB free disk space

### Building
- [ ] Run: `make -f Makefile.deb-i586 deb-i586-complete`
- [ ] Wait for completion
- [ ] Check output in `pkgout/`

### After Building
- [ ] Verify: `cd pkgout && bash verify-debian-i586.sh i386 vitte 2.1.1`
- [ ] Review: `cat pkgout/RELEASE-DEBIAN-I586-2.1.1.md`

---

## 🆘 Troubleshooting

**"gcc -m32 failed"**
```bash
# Install 32-bit compiler support
sudo apt install gcc-multilib g++-multilib
```

**"libc6:i386 not found"**
```bash
# Install 32-bit libc
sudo apt install libc6:i386
```

**"Build failed"**
```bash
make -f Makefile.deb-i586 deb-i586-clean
make -f Makefile.deb-i586 deb-i586-complete
```

---

## 💡 Customization

### More Parallel Jobs
```bash
JOBS=8 make -f Makefile.deb-i586 deb-i586-complete
```

### Maximum Optimization
```bash
OPT_LEVEL=3 make -f Makefile.deb-i586 deb-i586
```

### Custom Output
```bash
OUT_DIR=~/releases make -f Makefile.deb-i586 deb-i586-complete
```

---

## 🌍 Supported Platforms (i586 Pentium)

- ✓ Debian 10+ (32-bit Pentium)
- ✓ Ubuntu 18.04+ (32-bit Pentium)
- ✓ Linux Mint (32-bit Pentium)
- ✓ Any Debian-based i586 system
- ✓ Pentium (original) processors
- ✓ Pentium MMX processors

---

## 🔗 Quick Links

### Documentation
- [Building Guide](DEBIAN_BUILD_GUIDE_I586.md) - Use for reference
- [Platform Comparison](PLATFORM_BUILD_GUIDE.md) - See all platforms

### Related Architectures
- 64-bit (amd64): `make -f Makefile.deb help`
- 32-bit i686: `make -f Makefile.deb-i686 help`
- macOS: `make -f Makefile.pkg help`

---

## 🎓 Example Workflows

### Basic Build (i586 Pentium)
```bash
make -f Makefile.deb-i586 deb-i586-complete
```

### Fast Build (8 jobs)
```bash
JOBS=8 make -f Makefile.deb-i586 deb-i586-complete
```

### Highly Optimized
```bash
OPT_LEVEL=3 make -f Makefile.deb-i586 deb-i586
```

### With Custom Output
```bash
OUT_DIR=~/releases make -f Makefile.deb-i586 deb-i586-complete
```

---

## 🚀 Next Steps

**1. Build i586 Package**
```bash
make -f Makefile.deb-i586 deb-i586-complete
```

**2. Verify**
```bash
cd pkgout && bash verify-debian-i586.sh i386 vitte 2.1.1
```

**3. Distribute**
- Upload to Pentium system repositories
- Provide to Pentium/Pentium MMX users
- Archive for legacy system support

---

_Created: 2024-06-05 | Vitte 2.1.1+ | Debian i586 (Pentium 32-bit)_
