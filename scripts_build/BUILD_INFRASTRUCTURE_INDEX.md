# Vitte Multi-Platform Build Infrastructure 🚀

**Complete packaging infrastructure for macOS, Debian/Ubuntu 64-bit, and Debian/Ubuntu 32-bit**

---

## 🎯 Quick Navigation

### 🍎 macOS x86_64 (Intel)

**For:** Apple Mac systems (10.13+)

- **Quick Start:** [PKG_BUILD_INDEX.md](PKG_BUILD_INDEX.md)
- **Detailed Guide:** [PACKAGE_BUILD_GUIDE.md](PACKAGE_BUILD_GUIDE.md)
- **Build Command:** `make -f Makefile.pkg pkg-complete`
- **Output:** `.pkg` installer (Universal x86_64)

---

### 🐧 Debian/Ubuntu 64-bit (x86_64 / amd64)

**For:** Modern 64-bit Linux systems

- **Quick Start:** [DEBIAN_BUILD_INDEX.md](DEBIAN_BUILD_INDEX.md)
- **Detailed Guide:** [DEBIAN_BUILD_GUIDE.md](DEBIAN_BUILD_GUIDE.md)
- **Build Command:** `make -f Makefile.deb deb-complete`
- **Output:** `.deb` installer (amd64 architecture)

---

### 🐧 Debian/Ubuntu 32-bit (i686 / i386)

**For:** Legacy 32-bit systems, IoT, embedded devices

- **Quick Start:** [DEBIAN_BUILD_I686_INDEX.md](DEBIAN_BUILD_I686_INDEX.md)
- **Detailed Guide:** [DEBIAN_BUILD_GUIDE_I686.md](DEBIAN_BUILD_GUIDE_I686.md)
- **Build Command:** `make -f Makefile.deb-i686 deb-i686-complete`
- **Output:** `.deb` installer (i386 architecture)

---

## 📊 Platform Comparison

See [PLATFORM_BUILD_GUIDE.md](PLATFORM_BUILD_GUIDE.md) for detailed comparison of all three platforms.

| Feature | macOS | Debian 64-bit | Debian 32-bit |
|---------|-------|---------------|---------------|
| **Makefile** | `Makefile.pkg` | `Makefile.deb` | `Makefile.deb-i686` |
| **Architecture** | x86_64 | amd64 | i386 |
| **Output Format** | .pkg | .deb | .deb |
| **Target OS** | macOS 10.13+ | Debian 10+ | Debian 10+ |
| **Status** | ✅ Complete | ✅ Complete | ✅ Complete |

---

## 🗂️ File Structure

### Build Scripts (`scripts/`)

```
macOS:
├── build-macos-x86_64-universal-pkg.sh
├── build-distribution-bundle.sh
└── build-complete-pkg.sh

Debian 64-bit:
├── build-debian-x86_64-deb.sh
├── build-debian-distribution-bundle.sh
└── build-debian-complete-pkg.sh

Debian 32-bit:
├── build-debian-i686-deb.sh
├── build-debian-i686-distribution-bundle.sh
└── build-debian-i686-complete-pkg.sh
```

### Configuration (`Makefile.*`)

```
Makefile.pkg              (macOS build interface)
Makefile.deb              (Debian 64-bit build interface)
Makefile.deb-i686         (Debian 32-bit build interface)
```

### Documentation

```
macOS:
├── PKG_BUILD_INDEX.md              (quick start)
└── PACKAGE_BUILD_GUIDE.md          (detailed guide)

Debian 64-bit:
├── DEBIAN_BUILD_INDEX.md           (quick start)
└── DEBIAN_BUILD_GUIDE.md           (detailed guide)

Debian 32-bit:
├── DEBIAN_BUILD_I686_INDEX.md      (quick start)
└── DEBIAN_BUILD_GUIDE_I686.md      (detailed guide)

Common:
├── PLATFORM_BUILD_GUIDE.md         (all platforms)
└── BUILD_INFRASTRUCTURE_INDEX.md   (this file)
```

### Summary Files

```
PACKAGE_BUILD_SUMMARY.txt          (macOS overview)
DEBIAN_BUILD_SUMMARY.txt           (64-bit overview)
DEBIAN_BUILD_SUMMARY_I686.txt      (32-bit overview)
PLATFORM_BUILD_GUIDE.md            (cross-platform)
```

---

## 🚀 Getting Started

### Choose Your Platform

**1. I have a Mac:**
```bash
make -f Makefile.pkg pkg-complete
```
→ See: [PKG_BUILD_INDEX.md](PKG_BUILD_INDEX.md)

**2. I have a 64-bit Linux (modern):**
```bash
make -f Makefile.deb deb-complete
```
→ See: [DEBIAN_BUILD_INDEX.md](DEBIAN_BUILD_INDEX.md)

**3. I have a 32-bit Linux (legacy):**
```bash
make -f Makefile.deb-i686 deb-i686-complete
```
→ See: [DEBIAN_BUILD_I686_INDEX.md](DEBIAN_BUILD_I686_INDEX.md)

### View Available Options

```bash
# macOS
make -f Makefile.pkg help

# Debian 64-bit
make -f Makefile.deb help

# Debian 32-bit
make -f Makefile.deb-i686 help
```

---

## 📋 Complete Build Workflow

### Example: Full 64-bit Debian Build

```bash
# 1. Build with 8 parallel jobs and maximum optimization
JOBS=8 OPT_LEVEL=3 make -f Makefile.deb deb-complete

# 2. Verify output
cd pkgout && bash verify-debian-x86_64.sh amd64 vitte 2.1.1

# 3. Review installation guide
cat INSTALL-DEBIAN-X86_64.md

# 4. Install (if on compatible system)
sudo dpkg -i vitte_2.1.1_amd64.deb
```

### Example: Full 32-bit Debian Build

```bash
# 1. Build with custom output
OUT_DIR=~/releases make -f Makefile.deb-i686 deb-i686-complete

# 2. Verify
cd ~/releases && bash verify-debian-i686.sh i386 vitte 2.1.1

# 3. Review
cat INSTALL-DEBIAN-I686.md

# 4. Distribute
upload *.deb to repository
```

---

## 🎯 Common Tasks

### Build and Verify (Recommended Workflow)

```bash
# Build
make -f Makefile.deb deb-complete

# Verify
cd pkgout && bash verify-debian-x86_64.sh amd64 vitte 2.1.1

# Check files
ls -lh pkgout/
```

### Build with Customization

```bash
# Faster (more jobs)
JOBS=$(nproc) make -f Makefile.deb deb-complete

# Optimized binary
OPT_LEVEL=3 make -f Makefile.deb deb

# Custom output
OUT_DIR=~/releases make -f Makefile.deb deb-complete

# Combine all
JOBS=8 OPT_LEVEL=3 OUT_DIR=~/releases make -f Makefile.deb deb-complete
```

### Clean and Rebuild

```bash
# Remove artifacts
make -f Makefile.deb deb-clean

# Rebuild from scratch
make -f Makefile.deb deb-complete
```

---

## 📚 Documentation Quick Links

### For Beginners

- **Which platform to choose?** → [PLATFORM_BUILD_GUIDE.md](PLATFORM_BUILD_GUIDE.md)
- **I have a Mac** → [PKG_BUILD_INDEX.md](PKG_BUILD_INDEX.md)
- **I have 64-bit Linux** → [DEBIAN_BUILD_INDEX.md](DEBIAN_BUILD_INDEX.md)
- **I have 32-bit Linux** → [DEBIAN_BUILD_I686_INDEX.md](DEBIAN_BUILD_I686_INDEX.md)

### For Developers

- **macOS detailed guide** → [PACKAGE_BUILD_GUIDE.md](PACKAGE_BUILD_GUIDE.md)
- **64-bit detailed guide** → [DEBIAN_BUILD_GUIDE.md](DEBIAN_BUILD_GUIDE.md)
- **32-bit detailed guide** → [DEBIAN_BUILD_GUIDE_I686.md](DEBIAN_BUILD_GUIDE_I686.md)
- **Infrastructure summary** → [PLATFORM_BUILD_GUIDE.md](PLATFORM_BUILD_GUIDE.md)

### For Reference

- **macOS summary** → [PACKAGE_BUILD_SUMMARY.txt](PACKAGE_BUILD_SUMMARY.txt)
- **64-bit summary** → [DEBIAN_BUILD_SUMMARY.txt](DEBIAN_BUILD_SUMMARY.txt)
- **32-bit summary** → [DEBIAN_BUILD_SUMMARY_I686.txt](DEBIAN_BUILD_SUMMARY_I686.txt)

---

## ✅ Infrastructure Status

| Component | macOS | Debian 64-bit | Debian 32-bit |
|-----------|-------|---------------|---------------|
| **Build Scripts** | ✅ 3 files | ✅ 3 files | ✅ 3 files |
| **Makefile** | ✅ Complete | ✅ Complete | ✅ Complete |
| **Quick Start Docs** | ✅ Complete | ✅ Complete | ✅ Complete |
| **Detailed Guides** | ✅ Complete | ✅ Complete | ✅ Complete |
| **Summary Files** | ✅ Complete | ✅ Complete | ✅ Complete |
| **Platform Comparison** | ✅ Included | ✅ Included | ✅ Included |
| **Ready for Production** | ✅ YES | ✅ YES | ✅ YES |

---

## 🔧 Build System Features

### One-Command Builds

```bash
# macOS - everything in one command
make -f Makefile.pkg pkg-complete

# Debian 64-bit - everything in one command
make -f Makefile.deb deb-complete

# Debian 32-bit - everything in one command
make -f Makefile.deb-i686 deb-i686-complete
```

### Automatic Features

- ✅ Compiler detection and verification
- ✅ 32-bit support verification (i686)
- ✅ Parallel compilation (configurable)
- ✅ Optimization levels (0-3)
- ✅ SHA256 and SHA512 checksums
- ✅ Installation guides (per-platform)
- ✅ Package verification scripts
- ✅ Clean build procedures

### Customization Options

```bash
# All platforms support:
JOBS=N              # Parallel jobs (default: auto)
OPT_LEVEL=N         # Optimization 0-3 (default: 2)
OUT_DIR=path        # Output directory (default: pkgout)
VERSION=X.Y.Z       # Override version
```

---

## 🆘 Getting Help

### Quick Commands

```bash
# View available targets for your platform
make -f Makefile.pkg help          # macOS
make -f Makefile.deb help          # Debian 64-bit
make -f Makefile.deb-i686 help     # Debian 32-bit

# Read quick-start guide
cat PKG_BUILD_INDEX.md              # macOS
cat DEBIAN_BUILD_INDEX.md           # Debian 64-bit
cat DEBIAN_BUILD_I686_INDEX.md      # Debian 32-bit

# Read detailed guide
cat PACKAGE_BUILD_GUIDE.md          # macOS
cat DEBIAN_BUILD_GUIDE.md           # Debian 64-bit
cat DEBIAN_BUILD_GUIDE_I686.md      # Debian 32-bit
```

### Troubleshooting

Each guide includes comprehensive troubleshooting sections:
- **macOS:** [PACKAGE_BUILD_GUIDE.md#troubleshooting](PACKAGE_BUILD_GUIDE.md)
- **Debian 64-bit:** [DEBIAN_BUILD_GUIDE.md#troubleshooting](DEBIAN_BUILD_GUIDE.md)
- **Debian 32-bit:** [DEBIAN_BUILD_GUIDE_I686.md#troubleshooting](DEBIAN_BUILD_GUIDE_I686.md)

---

## 📊 Build Time Estimates

| Command | Time | Platform |
|---------|------|----------|
| `pkg-complete` | 5-15 min | macOS |
| `deb-complete` | 5-15 min | Debian 64-bit |
| `deb-i686-complete` | 5-15 min | Debian 32-bit |
| (with JOBS=8) | 2-8 min | Any platform |
| (with OPT_LEVEL=3) | +5 min | Any platform |

---

## 🌍 Supported Platforms

### macOS

- ✅ macOS 10.13+ (High Sierra and later)
- ✅ x86_64 architecture
- ✅ Xcode Command Line Tools required

### Debian/Ubuntu (64-bit)

- ✅ Debian 10+ (Buster and later)
- ✅ Ubuntu 18.04 LTS and later
- ✅ Linux Mint, Elementary OS
- ✅ Any Debian-based 64-bit distro

### Debian/Ubuntu (32-bit)

- ✅ Debian 10+ (32-bit version)
- ✅ Ubuntu 18.04 LTS+ (32-bit version)
- ✅ Linux Mint (32-bit)
- ✅ Any Debian-based 32-bit distro
- ✅ Legacy systems without 64-bit support

---

## 📝 Files Reference

### Essential Files

```
Makefile.pkg                    Main build configuration (macOS)
Makefile.deb                    Main build configuration (Debian 64-bit)
Makefile.deb-i686               Main build configuration (Debian 32-bit)

scripts/build-macos-x86_64-universal-pkg.sh
scripts/build-debian-x86_64-deb.sh
scripts/build-debian-i686-deb.sh
```

### Documentation

```
PKG_BUILD_INDEX.md              Quick start (macOS)
DEBIAN_BUILD_INDEX.md           Quick start (Debian 64-bit)
DEBIAN_BUILD_I686_INDEX.md      Quick start (Debian 32-bit)

PACKAGE_BUILD_GUIDE.md          Detailed (macOS)
DEBIAN_BUILD_GUIDE.md           Detailed (Debian 64-bit)
DEBIAN_BUILD_GUIDE_I686.md      Detailed (Debian 32-bit)

PLATFORM_BUILD_GUIDE.md         All platforms comparison
BUILD_INFRASTRUCTURE_INDEX.md   This file
```

---

## 🎓 Learning Path

### New to the Project?

1. Start: [PLATFORM_BUILD_GUIDE.md](PLATFORM_BUILD_GUIDE.md) - Understand your options
2. Choose your platform (macOS, 64-bit, or 32-bit)
3. Read the quick-start guide for your platform
4. Run your first build: `make -f Makefile.* *-complete`
5. Read detailed guide for customization

### Experienced Users?

1. Jump to your Makefile: `make -f Makefile.* help`
2. Use customization: `JOBS=8 OPT_LEVEL=3 make -f Makefile.* *-complete`
3. Reference detailed guide as needed

---

## 🚀 Next Steps

### First Time?

```bash
# Choose your platform from the options above
make -f Makefile.pkg pkg-complete              # macOS
# OR
make -f Makefile.deb deb-complete             # Debian 64-bit
# OR
make -f Makefile.deb-i686 deb-i686-complete   # Debian 32-bit
```

### Ready to Distribute?

```bash
# Verify your build
cd pkgout && bash verify-*.sh

# Review installation guides
cat INSTALL-*.md

# Upload files
# - .deb/.pkg (main package)
# - .sha256 and .sha512 (checksums)
# - INSTALL-*.md (documentation)
# - RELEASE-*.md (release notes)
```

---

## 📞 Support

- **Documentation:** See guides above
- **Issues:** https://github.com/vitte-lang/vitte/issues
- **Community:** https://vitte-lang.github.io/

---

## 🎉 Summary

You now have complete build infrastructure for Vitte across three major platforms:

- 🍎 **macOS x86_64** - Universal x86_64 packages
- 🐧 **Debian/Ubuntu 64-bit** - Modern Linux systems
- 🐧 **Debian/Ubuntu 32-bit** - Legacy and IoT devices

Each platform has:
- ✅ One-command build system
- ✅ Customizable build options
- ✅ Automatic verification
- ✅ Complete documentation
- ✅ Production-ready infrastructure

**Get started:** `make -f Makefile.pkg help` | `make -f Makefile.deb help` | `make -f Makefile.deb-i686 help`

---

_Vitte Multi-Platform Build Infrastructure | Complete & Production-Ready_
