# Vitte Debian/Ubuntu Package Build - Index & Getting Started

## 📖 Read First

Start here for a quick overview:

### 🚀 [Quick Start (This Page)](#quick-start)
Complete build in one command - scroll down!

### 📚 Complete Documentation

1. **[DEBIAN_BUILD_GUIDE.md](DEBIAN_BUILD_GUIDE.md)** - Comprehensive building guide
2. **[DEBIAN_BUILD_README.md](DEBIAN_BUILD_README.md)** - Overview & structure (if it exists)
3. **[scripts/build-debian-complete-pkg.sh](scripts/build-debian-complete-pkg.sh)** - Orchestrator

---

## 🚀 Quick Start

### Build Everything (5-15 minutes)

```bash
cd /path/to/vitte
make -f Makefile.deb deb-complete
```

That's it! This single command:
- ✅ Compiles Vitte for x86_64
- ✅ Bundles standard library
- ✅ Includes editor support
- ✅ Creates Debian .deb installer
- ✅ Generates checksums
- ✅ Creates installation guides

**Output:** `pkgout/vitte_2.1.1_amd64.deb` (ready to distribute!)

---

## 📋 What Was Created

### 🔧 Build Scripts (in `scripts/`)

| Script | Purpose | Size |
|--------|---------|------|
| `build-debian-x86_64-deb.sh` | Main compiler build & packaging | 10.4 KB |
| `build-debian-distribution-bundle.sh` | Create checksums & docs | 12.2 KB |
| `build-debian-complete-pkg.sh` | Orchestrate complete workflow | 5.6 KB |

### ⚙️ Build Configuration

| File | Purpose |
|------|---------|
| `Makefile.deb` | GNU Make targets for building |

### 📖 Documentation

| File | Description |
|------|-------------|
| `DEBIAN_BUILD_INDEX.md` | This file - quick navigation |
| `DEBIAN_BUILD_GUIDE.md` | Detailed building guide |
| (Add others as needed) | |

---

## 📁 Output Structure

After building, your `pkgout/` directory contains:

```
pkgout/
├── vitte_2.1.1_amd64.deb              # Main installer (~150-250 MB)
├── vitte_2.1.1_amd64.deb.sha256       # SHA256 checksum
├── vitte_2.1.1_amd64.deb.sha512       # SHA512 checksum
├── INSTALL-DEBIAN.md                  # Installation guide
├── QUICKSTART-DEBIAN.md               # Quick start guide
├── RELEASE-DEBIAN-2.1.1.md            # Release notes
├── PACKAGE_MANIFEST-DEBIAN.txt        # Package contents
└── verify-debian.sh                   # Verification script
```

---

## 🎯 Common Tasks

### Build the Package
```bash
make -f Makefile.deb deb-complete
```

### View Build Options
```bash
make -f Makefile.deb help
```

### Verify Package
```bash
cd pkgout && bash verify-debian.sh amd64 vitte 2.1.1
```

### Install Package
```bash
sudo apt install ./pkgout/vitte_2.1.1_amd64.deb
```

### Clean Build Artifacts
```bash
make -f Makefile.deb deb-clean
```

### Custom Build (More Jobs)
```bash
JOBS=8 make -f Makefile.deb deb-complete
```

---

## ✅ Checklist

### Before Building
- [ ] Debian 10+ or Ubuntu 18.04+
- [ ] x86_64 processor
- [ ] Install build tools: `sudo apt install build-essential dpkg-dev rsync`
- [ ] 1 GB free disk space

### Building
- [ ] Run: `make -f Makefile.deb deb-complete`
- [ ] Wait for completion (5-15 minutes)
- [ ] Check output in `pkgout/`

### After Building
- [ ] Verify: `cd pkgout && bash verify-debian.sh amd64 vitte 2.1.1`
- [ ] Review: `cat pkgout/RELEASE-DEBIAN-2.1.1.md`
- [ ] Test: `pkgout/verify-debian.sh amd64 vitte 2.1.1`

### Distribution
- [ ] Upload to GitHub releases
- [ ] Update website links
- [ ] Setup PPA if needed

---

## 🆘 Troubleshooting

**"dpkg-deb: command not found"**
```bash
sudo apt install dpkg-dev
```

**"build-essential not found"**
```bash
sudo apt install build-essential
```

**"Build failed"**
```bash
make -f Makefile.deb deb-clean
make -f Makefile.deb deb-complete
```

For more help, see **[DEBIAN_BUILD_GUIDE.md](DEBIAN_BUILD_GUIDE.md#troubleshooting)**

---

## 📚 Learning Path

### 1️⃣ New to Building
1. Read: **[DEBIAN_BUILD_GUIDE.md](DEBIAN_BUILD_GUIDE.md)** (10 min)
2. Build: `make -f Makefile.deb deb-complete` (15 min)

### 2️⃣ Want Details
1. Review: Makefile.deb targets
2. Check: Environment variables
3. Experiment with options

### 3️⃣ Need Custom Build
1. Review: Build script options
2. Modify: Environment variables
3. Try: Custom configurations

---

## 🔗 Quick Links

### Documentation
- [Building Guide](DEBIAN_BUILD_GUIDE.md) - Detailed instructions
- [Makefile](Makefile.deb) - Build targets

### Scripts
- [Main Build Script](scripts/build-debian-x86_64-deb.sh)
- [Distribution Script](scripts/build-debian-distribution-bundle.sh)
- [Orchestrator](scripts/build-debian-complete-pkg.sh)

---

## 🎓 Example Workflows

### Basic Build
```bash
make -f Makefile.deb deb-complete
```

### Fast Build (8 parallel jobs)
```bash
JOBS=8 make -f Makefile.deb deb-complete
```

### Optimized Build
```bash
OPT_LEVEL=3 make -f Makefile.deb deb-complete
```

### Custom Output
```bash
OUT_DIR=~/releases make -f Makefile.deb deb-complete
```

### Full Verification
```bash
make -f Makefile.deb deb-complete    # Build
make -f Makefile.deb verify-deb      # Verify
make -f Makefile.deb install-deb     # Test install
```

---

## 📊 Build Time Estimates

| Command | Time | Notes |
|---------|------|-------|
| `deb-complete` | 5-15 min | Full build (recommended) |
| `deb` | 10-12 min | Compiler build only |
| `deb-bundle` | 1-2 min | Distribution files |
| `verify-deb` | < 1 min | Checksum verification |

---

## 🎁 What You Get

After installation:

```
/usr/
├── bin/vitte                           Main compiler
└── share/vitte/
    ├── src/                            Source code
    ├── editors/                        Editor support
    ├── docs/                           Full documentation
    ├── completions/                    Shell completions
    └── ... (and much more!)
```

---

## 🚀 Next Steps

**1. Quick Build (Recommended)**
```bash
make -f Makefile.deb deb-complete
```

**2. Verify It Worked**
```bash
ls -lh pkgout/vitte-*.deb
```

**3. Verify Package**
```bash
cd pkgout && bash verify-debian.sh amd64 vitte 2.1.1
```

**4. Share It**
- GitHub: Upload to releases
- Website: Host for download
- Package Managers: Integration

---

## 📞 Getting Help

- **Building Guide:** [DEBIAN_BUILD_GUIDE.md](DEBIAN_BUILD_GUIDE.md)
- **Issues:** https://github.com/vitte-lang/vitte/issues
- **Docs:** https://vitte-lang.github.io/

---

**Ready?** Start with:
```bash
make -f Makefile.deb deb-complete
```

**Questions?** See [DEBIAN_BUILD_GUIDE.md](DEBIAN_BUILD_GUIDE.md)

---

_Created: 2024-06-05 | Vitte 2.1.1+ | Debian/Ubuntu x86_64_
