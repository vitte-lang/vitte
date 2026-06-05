# Vitte macOS Package Build - Index & Getting Started

## 📖 Read First

Start here for a quick overview:

### 🚀 [Quick Start (This Page)](#quick-start)
Complete build in one command - scroll down!

### 📚 Complete Documentation

1. **[PACKAGE_BUILD_README.md](PACKAGE_BUILD_README.md)** - Overview & directory structure
2. **[PACKAGE_BUILD_GUIDE.md](PACKAGE_BUILD_GUIDE.md)** - Comprehensive building guide  
3. **[PACKAGE_BUILD_SUMMARY.txt](PACKAGE_BUILD_SUMMARY.txt)** - Infrastructure summary
4. **[scripts/pkg-help.sh](scripts/pkg-help.sh)** - Quick reference (run: `bash scripts/pkg-help.sh`)

---

## 🚀 Quick Start

### Build Everything (5-15 minutes)

```bash
cd /path/to/vitte
make -f Makefile.pkg pkg-complete
```

That's it! This single command:
- ✅ Compiles Vitte for x86_64
- ✅ Bundles standard library
- ✅ Includes editor support
- ✅ Creates macOS .pkg installer
- ✅ Generates checksums
- ✅ Creates installation guides

**Output:** `pkgout/vitte-2.1.1.pkg` (ready to distribute!)

---

## 📋 What Was Created

### 🔧 Build Scripts (in `scripts/`)

| Script | Purpose | Size |
|--------|---------|------|
| `build-macos-x86_64-universal-pkg.sh` | Main compiler build & packaging | 10.6 KB |
| `build-distribution-bundle.sh` | Create checksums & docs | 10.3 KB |
| `build-complete-pkg.sh` | Orchestrate complete workflow | 5.6 KB |
| `pkg-help.sh` | Quick reference guide | 2.5 KB |

### ⚙️ Build Configuration

| File | Purpose |
|------|---------|
| `Makefile.pkg` | GNU Make targets for building |

### 📖 Documentation

| File | Description |
|------|-------------|
| `PKG_BUILD_INDEX.md` | This file - quick navigation |
| `PACKAGE_BUILD_README.md` | Overview & directory structure |
| `PACKAGE_BUILD_GUIDE.md` | Detailed building guide |
| `PACKAGE_BUILD_SUMMARY.txt` | Infrastructure summary |

---

## 📁 Output Structure

After building, your `pkgout/` directory contains:

```
pkgout/
├── vitte-2.1.1.pkg              # Main installer (~150-200 MB)
├── vitte-2.1.1.pkg.sha256       # SHA256 checksum
├── vitte-2.1.1.pkg.sha512       # SHA512 checksum
├── INSTALL.md                   # Installation guide
├── QUICKSTART.md                # Quick start tutorial
├── RELEASE-2.1.1.md             # Release notes
├── PACKAGE_MANIFEST.txt         # Complete manifest
└── verify.sh                     # Verification script
```

---

## 🎯 Common Tasks

### Build the Package
```bash
make -f Makefile.pkg pkg-complete
```

### View Build Options
```bash
make -f Makefile.pkg help
```

### Verify Package
```bash
cd pkgout && bash verify.sh vitte-2.1.1
```

### Install Package
```bash
sudo installer -pkg pkgout/vitte-2.1.1.pkg -target /
```

### Clean Build Artifacts
```bash
make -f Makefile.pkg pkg-clean
```

### Custom Build (More Jobs)
```bash
JOBS=8 make -f Makefile.pkg pkg-complete
```

### View Quick Reference
```bash
bash scripts/pkg-help.sh
```

---

## 🔍 File Guide

### For End Users
- **After build:** Read `pkgout/INSTALL.md` and `pkgout/QUICKSTART.md`

### For Developers
- **Quick overview:** Read `PACKAGE_BUILD_README.md`
- **Building guide:** Read `PACKAGE_BUILD_GUIDE.md`
- **Infrastructure:** Read `PACKAGE_BUILD_SUMMARY.txt`

### For Package Maintainers
- **Detailed instructions:** `PACKAGE_BUILD_GUIDE.md`
- **Distribution workflow:** `PACKAGE_BUILD_README.md` (Distribution section)
- **Scripting reference:** `Makefile.pkg`

---

## ✅ Checklist

### Before Building
- [ ] macOS 10.13 or later
- [ ] x86_64 processor
- [ ] Install Xcode Command Line Tools: `xcode-select --install`
- [ ] 1 GB free disk space

### Building
- [ ] Run: `make -f Makefile.pkg pkg-complete`
- [ ] Wait for completion (5-15 minutes)
- [ ] Check output in `pkgout/`

### After Building
- [ ] Verify: `cd pkgout && bash verify.sh vitte-2.1.1`
- [ ] Review: `cat pkgout/RELEASE-2.1.1.md`
- [ ] Test: `pkgout/verify.sh vitte-2.1.1`

### Distribution
- [ ] Upload to GitHub releases
- [ ] Update website links
- [ ] Announce availability

---

## 🆘 Troubleshooting

**"xcode-select: error"**
```bash
xcode-select --install
```

**"rsync not found"**
```bash
brew install rsync
```

**"Build failed"**
```bash
make -f Makefile.pkg pkg-clean
make -f Makefile.pkg pkg-complete
```

For more help, see **[PACKAGE_BUILD_GUIDE.md](PACKAGE_BUILD_GUIDE.md#troubleshooting)**

---

## 📚 Learning Path

### 1️⃣ New to Building
1. Read: **[PACKAGE_BUILD_README.md](PACKAGE_BUILD_README.md)** (5 min)
2. Run: `bash scripts/pkg-help.sh` (2 min)
3. Build: `make -f Makefile.pkg pkg-complete` (15 min)

### 2️⃣ Want Details
1. Read: **[PACKAGE_BUILD_GUIDE.md](PACKAGE_BUILD_GUIDE.md)** (10 min)
2. Read: **[PACKAGE_BUILD_SUMMARY.txt](PACKAGE_BUILD_SUMMARY.txt)** (5 min)
3. Experiment with options

### 3️⃣ Need Custom Build
1. Review: Makefile.pkg targets
2. Check: Environment variables (JOBS, OPT_LEVEL, OUT_DIR)
3. Try: Custom configurations

---

## 🔗 Quick Links

### Documentation
- [README](PACKAGE_BUILD_README.md) - Overview
- [Building Guide](PACKAGE_BUILD_GUIDE.md) - Detailed instructions
- [Summary](PACKAGE_BUILD_SUMMARY.txt) - Infrastructure overview

### Scripts
- [Main Build Script](scripts/build-macos-x86_64-universal-pkg.sh)
- [Distribution Script](scripts/build-distribution-bundle.sh)
- [Orchestrator](scripts/build-complete-pkg.sh)
- [Quick Help](scripts/pkg-help.sh)

### Configuration
- [Makefile](Makefile.pkg) - Build targets

---

## 🎓 Example Workflows

### Basic Build
```bash
make -f Makefile.pkg pkg-complete
```

### Fast Build (8 parallel jobs)
```bash
JOBS=8 make -f Makefile.pkg pkg-complete
```

### Optimized Build
```bash
OPT_LEVEL=3 make -f Makefile.pkg pkg-complete
```

### Custom Output
```bash
OUT_DIR=~/releases make -f Makefile.pkg pkg-complete
```

### Full Verification
```bash
make -f Makefile.pkg pkg-complete      # Build
make -f Makefile.pkg verify-pkg        # Verify
make -f Makefile.pkg install-pkg       # Test install
```

---

## 📊 Build Time Estimates

| Command | Time | Notes |
|---------|------|-------|
| `pkg-complete` | 5-15 min | Full build (recommended) |
| `pkg` | 10-12 min | Compiler build only |
| `pkg-bundle` | 1-2 min | Distribution files |
| `verify-pkg` | < 1 min | Checksum verification |

---

## 🎁 What You Get

After installation, users have:

```
/usr/local/
├── bin/vitte                           Main compiler
└── share/vitte/
    ├── src/                            Source code
    ├── editors/                        Editor support
    ├── docs/                           Full documentation
    ├── completions/                    Shell completions
    ├── uninstall.sh                    Uninstall script
    └── ... (and much more!)
```

---

## 🚀 Next Steps

**1. Quick Build (Recommended)**
```bash
make -f Makefile.pkg pkg-complete
```

**2. Verify It Worked**
```bash
ls -lh pkgout/vitte-*.pkg
```

**3. Verify Package**
```bash
cd pkgout && bash verify.sh vitte-2.1.1
```

**4. Share It**
- GitHub: Upload to releases
- Website: Host for download
- Package Managers: Integration

---

## 📞 Getting Help

- **Quick Reference:** `bash scripts/pkg-help.sh`
- **Building Guide:** [PACKAGE_BUILD_GUIDE.md](PACKAGE_BUILD_GUIDE.md)
- **Issues:** https://github.com/vitte-lang/vitte/issues
- **Docs:** https://vitte-lang.github.io/

---

**Ready?** Start with:
```bash
make -f Makefile.pkg pkg-complete
```

**Questions?** See [PACKAGE_BUILD_GUIDE.md](PACKAGE_BUILD_GUIDE.md)

---

_Created: 2024-06-05 | Vitte 2.1.1+ | macOS x86_64_
