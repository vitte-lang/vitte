# Vitte macOS i686 (32-bit, iMac 2006+) - Quick Start

## 📖 Overview

Build infrastructure for **macOS i686 (32-bit)** - compatible with iMac 2006+ Intel models.

### 🎯 Use This If You Have:
- **iMac 2006-2007** (Intel Core)
- **Mac mini 2006+** (Intel Core)
- **MacBook 2006+** (Intel)
- **32-bit macOS support** needed

---

## 🚀 Quick Start

```bash
make -f Makefile.pkg-i686 pkg-i686-complete
```

---

## 📋 Targets

- `pkg-i686-complete` - Full build (recommended)
- `pkg-i686` - Build .pkg only
- `pkg-i686-bundle` - Distribution files
- `verify-pkg-i686` - Verify checksums
- `pkg-i686-clean` - Clean artifacts

---

## ✅ System Requirements

### macOS Version
- macOS 10.4 Tiger+ (32-bit support)
- macOS 10.5 Leopard (iMac 2006 minimum)
- macOS 10.7 Lion (last with full 32-bit support)

### Hardware
- Intel Core processor (i686 32-bit)
- iMac 2006+ Intel models
- Mac mini 2006+ Intel
- MacBook 2006+ Intel

---

## 🎉 Output

```
pkgout/vitte-2.1.1-i686.pkg          (32-bit installer)
pkgout/vitte-2.1.1-i686.pkg.sha256   (checksum)
```

---

_Vitte 2.1.1+ | macOS i686 (32-bit) | iMac 2006+ Support_
