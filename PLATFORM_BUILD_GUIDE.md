# Vitte Platform-Specific Package Builds

This repository now includes complete build infrastructure for both **macOS** and **Debian/Ubuntu**.

## Quick Navigation

### macOS x86_64 Package
- **Index:** [`PKG_BUILD_INDEX.md`](PKG_BUILD_INDEX.md)
- **Guide:** [`PACKAGE_BUILD_GUIDE.md`](PACKAGE_BUILD_GUIDE.md)
- **Build:** `make -f Makefile.pkg pkg-complete`
- **Output:** `pkgout/vitte-*.pkg`

### Debian/Ubuntu x86_64 Package
- **Index:** [`DEBIAN_BUILD_INDEX.md`](DEBIAN_BUILD_INDEX.md)
- **Guide:** [`DEBIAN_BUILD_GUIDE.md`](DEBIAN_BUILD_GUIDE.md)
- **Build:** `make -f Makefile.deb deb-complete`
- **Output:** `pkgout/vitte_*.deb`

---

## Comparison

| Feature | macOS | Debian/Ubuntu |
|---------|-------|---------------|
| **Build Command** | `make -f Makefile.pkg pkg-complete` | `make -f Makefile.deb deb-complete` |
| **Output Format** | `.pkg` (macOS installer) | `.deb` (Debian package) |
| **Architecture** | x86_64 universal | x86_64 (amd64) |
| **Installation** | Double-click or `installer` | `apt install` or `dpkg -i` |
| **Prerequisites** | Xcode Command Line Tools | `build-essential` |
| **Documentation File** | `PACKAGE_BUILD_GUIDE.md` | `DEBIAN_BUILD_GUIDE.md` |
| **Quick Start File** | `PKG_BUILD_INDEX.md` | `DEBIAN_BUILD_INDEX.md` |

---

## Platform-Specific Build Directories

### macOS Build Files
```
scripts/
├── build-macos-x86_64-universal-pkg.sh
├── build-distribution-bundle.sh
└── build-complete-pkg.sh

Makefile.pkg

Documentation/
├── PKG_BUILD_INDEX.md
├── PACKAGE_BUILD_README.md
├── PACKAGE_BUILD_GUIDE.md
└── PACKAGE_BUILD_SUMMARY.txt
```

### Debian Build Files
```
scripts/
├── build-debian-x86_64-deb.sh
├── build-debian-distribution-bundle.sh
└── build-debian-complete-pkg.sh

Makefile.deb

Documentation/
├── DEBIAN_BUILD_INDEX.md
├── DEBIAN_BUILD_GUIDE.md
└── DEBIAN_BUILD_SUMMARY.txt
```

---

## Quick Build Guide

### For macOS

```bash
# Install prerequisites
xcode-select --install

# Build
make -f Makefile.pkg pkg-complete

# Verify
cd pkgout && bash verify.sh vitte-2.1.1

# Install (optional)
sudo installer -pkg pkgout/vitte-2.1.1.pkg -target /
```

### For Debian/Ubuntu

```bash
# Install prerequisites
sudo apt install build-essential dpkg-dev rsync

# Build
make -f Makefile.deb deb-complete

# Verify
cd pkgout && bash verify-debian.sh amd64 vitte 2.1.1

# Install (optional)
sudo apt install ./pkgout/vitte_2.1.1_amd64.deb
```

---

## Supported Platforms

### macOS
- macOS 10.13 (High Sierra) or later
- x86_64 processor (Intel)
- Requires Xcode Command Line Tools

### Debian/Ubuntu
- Debian 10 (Buster) or later
- Ubuntu 18.04 LTS or later
- Linux Mint, Elementary OS, Pop!_OS, etc.
- x86_64 processor

---

## Generated Output Structure

### macOS (pkgout/)
```
vitte-2.1.1.pkg              # Installer package
vitte-2.1.1.pkg.sha256       # Checksum
INSTALL.md                   # Installation guide
QUICKSTART.md                # Quick start
RELEASE-2.1.1.md            # Release notes
```

### Debian/Ubuntu (pkgout/)
```
vitte_2.1.1_amd64.deb        # Installer package
vitte_2.1.1_amd64.deb.sha256 # Checksum
INSTALL-DEBIAN.md            # Installation guide
QUICKSTART-DEBIAN.md         # Quick start
RELEASE-DEBIAN-2.1.1.md     # Release notes
```

---

## Environment Variables

Both platforms support:

```bash
# Output directory (default: pkgout)
OUT_DIR=/custom/path

# Package version (default: from PACKAGE_VERSION file)
VERSION=2.1.1

# Number of parallel build jobs (default: auto-detected)
JOBS=8

# Optimization level (default: 2)
OPT_LEVEL=3

# For Debian only: Architecture
ARCH=amd64
```

---

## Build Time Estimates

| Platform | Command | Time |
|----------|---------|------|
| macOS | `pkg-complete` | 5-15 min |
| macOS | `pkg` | 10-12 min |
| Debian | `deb-complete` | 5-15 min |
| Debian | `deb` | 10-12 min |

---

## Documentation Files

### macOS Documentation
1. **PKG_BUILD_INDEX.md** - Start here (quick overview)
2. **PACKAGE_BUILD_README.md** - Overview & structure
3. **PACKAGE_BUILD_GUIDE.md** - Detailed building guide
4. **PACKAGE_BUILD_SUMMARY.txt** - Infrastructure summary

### Debian Documentation
1. **DEBIAN_BUILD_INDEX.md** - Start here (quick overview)
2. **DEBIAN_BUILD_GUIDE.md** - Detailed building guide
3. **DEBIAN_BUILD_SUMMARY.txt** - Infrastructure summary

### Common Documentation
- **PLATFORM_BUILD_GUIDE.md** - This file (platform comparison)
- **PKG_BUILD_INDEX.md** - macOS quick reference
- **DEBIAN_BUILD_INDEX.md** - Debian quick reference

---

## Distribution

### macOS Distribution
- GitHub Releases (upload .pkg file)
- DMG image (optional)
- Website hosting
- macOS app stores

### Debian Distribution
- GitHub Releases (upload .deb file)
- Ubuntu PPA repositories
- Debian repositories
- Linux package mirrors
- Custom apt repositories

---

## Next Steps

### For macOS Development
1. Read: [`PKG_BUILD_INDEX.md`](PKG_BUILD_INDEX.md)
2. Build: `make -f Makefile.pkg pkg-complete`
3. Distribute: Upload to GitHub/website

### For Debian Development
1. Read: [`DEBIAN_BUILD_INDEX.md`](DEBIAN_BUILD_INDEX.md)
2. Build: `make -f Makefile.deb deb-complete`
3. Distribute: Upload to GitHub/PPA

### For Both Platforms
1. Build macOS: `make -f Makefile.pkg pkg-complete`
2. Build Debian: `make -f Makefile.deb deb-complete`
3. Verify both packages
4. Create combined release with both platform packages

---

## Troubleshooting

### macOS Issues
See: [`PACKAGE_BUILD_GUIDE.md#troubleshooting`](PACKAGE_BUILD_GUIDE.md#troubleshooting)

### Debian Issues
See: [`DEBIAN_BUILD_GUIDE.md#troubleshooting`](DEBIAN_BUILD_GUIDE.md#troubleshooting)

---

## Support

- **GitHub:** https://github.com/vitte-lang/vitte
- **Issues:** https://github.com/vitte-lang/vitte/issues
- **Documentation:** https://vitte-lang.github.io/

---

## License

Vitte is licensed under the MIT License.

---

**Last Updated:** 2024-06-05
**Vitte Version:** 2.1.1+
**Platforms:** macOS x86_64, Debian/Ubuntu x86_64
