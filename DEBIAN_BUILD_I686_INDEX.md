# Vitte Debian i686 (32-bit) Package Build - Index & Getting Started

## 📖 Quick Overview

This is the **32-bit (i686)** build infrastructure for Debian/Ubuntu systems.

### 🎯 Use This If You Have:
- **32-bit Debian/Ubuntu system**
- **32-bit i686 processor**
- **Legacy system without 64-bit support**
- **Need to distribute to 32-bit users**

### ⚠️ Use 64-bit Instead If You Have:
- **Modern 64-bit system** → Use `DEBIAN_BUILD_INDEX.md` (amd64)
- **macOS system** → Use `PKG_BUILD_INDEX.md`

---

## 🚀 Quick Start

### Build i686 Package (5-15 minutes)

```bash
cd /path/to/vitte
make -f Makefile.deb-i686 deb-i686-complete
```

This automatically:
- ✅ Compiles Vitte for i686 (32-bit)
- ✅ Bundles standard library
- ✅ Includes editor support
- ✅ Creates Debian .deb installer
- ✅ Generates checksums
- ✅ Creates installation guides

**Output:** `pkgout/vitte_2.1.1_i386.deb` (32-bit version)

---

## 📋 What Was Created

### 🔧 Build Scripts (in `scripts/`)

| Script | Purpose |
|--------|---------|
| `build-debian-i686-deb.sh` | Main compiler build & packaging (32-bit) |
| `build-debian-i686-distribution-bundle.sh` | Create checksums & docs (32-bit) |
| `build-debian-i686-complete-pkg.sh` | Orchestrate complete workflow |

### ⚙️ Build Configuration

| File | Purpose |
|------|---------|
| `Makefile.deb-i686` | GNU Make targets for i686 builds |

---

## 📁 Output Structure

After building:

```
pkgout/
├── vitte_2.1.1_i386.deb              (32-bit installer)
├── vitte_2.1.1_i386.deb.sha256       (checksum)
├── vitte_2.1.1_i386.deb.sha512       (checksum)
├── INSTALL-DEBIAN-I686.md            (i686 guide)
├── RELEASE-DEBIAN-I686-2.1.1.md      (i686 notes)
└── verify-debian-i686.sh             (i686 verification)
```

---

## 🎯 Common Tasks

### Build the Package
```bash
make -f Makefile.deb-i686 deb-i686-complete
```

### View Build Options
```bash
make -f Makefile.deb-i686 help
```

### Verify Package (32-bit)
```bash
cd pkgout && bash verify-debian-i686.sh i386 vitte 2.1.1
```

### Install Package (32-bit)
```bash
sudo dpkg -i pkgout/vitte_2.1.1_i386.deb
sudo apt install -f  # If dependencies needed
```

### Clean Build Artifacts
```bash
make -f Makefile.deb-i686 deb-i686-clean
```

---

## ✅ Checklist

### Before Building
- [ ] Debian 10+ or Ubuntu 18.04+ (32-bit)
- [ ] i686 processor
- [ ] Build tools: `sudo apt install build-essential gcc-multilib`
- [ ] 32-bit compiler: `sudo apt install gcc-multilib`
- [ ] 1 GB free disk space

### Building
- [ ] Run: `make -f Makefile.deb-i686 deb-i686-complete`
- [ ] Wait for completion
- [ ] Check output in `pkgout/`

### After Building
- [ ] Verify: `cd pkgout && bash verify-debian-i686.sh i386 vitte 2.1.1`
- [ ] Review: `cat pkgout/RELEASE-DEBIAN-I686-2.1.1.md`

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
make -f Makefile.deb-i686 deb-i686-clean
make -f Makefile.deb-i686 deb-i686-complete
```

---

## 💡 Customization

### More Parallel Jobs
```bash
JOBS=8 make -f Makefile.deb-i686 deb-i686-complete
```

### Maximum Optimization
```bash
OPT_LEVEL=3 make -f Makefile.deb-i686 deb-i686
```

### Custom Output
```bash
OUT_DIR=~/releases make -f Makefile.deb-i686 deb-i686-complete
```

---

## 📊 Build Time Estimates

| Command | Time | Notes |
|---------|------|-------|
| `deb-i686-complete` | 5-15 min | Full build (recommended) |
| `deb-i686` | 10-12 min | Compiler build only |
| `deb-i686-bundle` | 1-2 min | Distribution files |
| `verify-deb-i686` | < 1 min | Checksum verification |

---

## 🌍 Supported Platforms (32-bit)

- ✓ Debian 10 (Buster) 32-bit
- ✓ Debian 11 (Bullseye) 32-bit
- ✓ Debian 12 (Bookworm) 32-bit
- ✓ Ubuntu 18.04 LTS 32-bit
- ✓ Ubuntu 20.04 LTS 32-bit
- ✓ Ubuntu 22.04 LTS 32-bit
- ✓ Linux Mint 32-bit
- ✓ Elementary OS 32-bit
- ✓ Other Debian-based 32-bit distros

---

## 🔗 Quick Links

### Documentation
- [Building Guide](DEBIAN_BUILD_GUIDE.md) - Use for reference
- [Platform Comparison](PLATFORM_BUILD_GUIDE.md) - See all platforms

### Make Targets
- 64-bit (amd64): `make -f Makefile.deb help`
- 32-bit (i386): `make -f Makefile.deb-i686 help`
- macOS: `make -f Makefile.pkg help`

---

## 🎓 Example Workflows

### Basic Build (32-bit)
```bash
make -f Makefile.deb-i686 deb-i686-complete
```

### Fast Build (8 jobs)
```bash
JOBS=8 make -f Makefile.deb-i686 deb-i686-complete
```

### Highly Optimized
```bash
OPT_LEVEL=3 make -f Makefile.deb-i686 deb-i686
```

### With Custom Output
```bash
OUT_DIR=~/releases make -f Makefile.deb-i686 deb-i686-complete
```

---

## 🚀 Next Steps

**1. Build i686 Package**
```bash
make -f Makefile.deb-i686 deb-i686-complete
```

**2. Verify**
```bash
cd pkgout && bash verify-debian-i686.sh i386 vitte 2.1.1
```

**3. Distribute**
- Upload to GitHub releases
- Host on 32-bit package repositories
- Provide to legacy system users

---

## 📞 Getting Help

- **Makefile targets:** `make -f Makefile.deb-i686 help`
- **Issues:** https://github.com/vitte-lang/vitte/issues
- **Docs:** https://vitte-lang.github.io/

---

**Ready?** Start with:
```bash
make -f Makefile.deb-i686 deb-i686-complete
```

---

_Created: 2024-06-05 | Vitte 2.1.1+ | Debian i686 (32-bit)_
