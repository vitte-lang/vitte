# Language Integration Guide for Build Scripts

This document describes how to integrate the language selection system into Vitte build scripts.

## Overview

The language integration consists of three main components:

1. **configure-language.sh** - Interactive language selector
2. **package-locales.sh** - Packages locale files for distribution
3. **Debian configuration** - debconf templates, config, and postinst scripts

## Integration Steps

### For Debian .deb Builds

#### 1. Add locale packaging to staging directory

In `scripts/build-debian-*.sh`, after copying documentation and editors:

```bash
log "Copying localization files..."
bash "$ROOT_DIR/scripts/package-locales.sh" \
  || warn "Failed to package locales (optional)"
  
# Export variables for package-locales.sh
export ROOT_DIR
export STAGE_ROOT="$stage_root"
export TARGET_LOCALE_DIR="/usr/share/vitte/locales"
export INSTALL_TYPE="debian"
```

#### 2. Update Debian control file

Add `debconf` as a dependency in `create_debian_control()`:

```bash
Depends: debconf (>= 0.5)
```

#### 3. Use new postinst and config scripts

In `create_postinst_script()` and `create_prerm_script()`:

```bash
# Copy debconf scripts
if [ -f "$ROOT_DIR/toolchain/scripts/package/debian/config" ]; then
  install -m 0755 "$ROOT_DIR/toolchain/scripts/package/debian/config" \
    "$debian_dir/config"
fi

if [ -f "$ROOT_DIR/toolchain/scripts/package/debian/postinst" ]; then
  install -m 0755 "$ROOT_DIR/toolchain/scripts/package/debian/postinst" \
    "$debian_dir/postinst"
else
  # Use default postinst template
  create_postinst_script "$debian_dir/postinst"
fi
```

### For macOS .pkg Builds

#### 1. Package locales for macOS

In `scripts/build-macos-*.sh`, create language component packages:

```bash
log "Packaging language components..."
export ROOT_DIR
export STAGE_ROOT="$stage_root"
export TARGET_LOCALE_DIR="/usr/local/vitte/locales"
export INSTALL_TYPE="macos"

bash "$ROOT_DIR/scripts/package-locales.sh"
```

#### 2. Use Distribution.xml

Update `productbuild` command to use the new Distribution.xml:

```bash
productbuild --distribution "$ROOT_DIR/scripts/installer/Distribution.xml" \
  --package-path "$PKG_COMPONENTS_DIR" \
  "$OUT_DIR/Vitte-$VERSION.pkg"
```

### For Other Platforms

The pattern is consistent across all platforms:

1. Set environment variables (ROOT_DIR, STAGE_ROOT, TARGET_LOCALE_DIR, INSTALL_TYPE)
2. Call `bash scripts/package-locales.sh`
3. Ensure locale files are in the final package

## Environment Variables

These variables control the behavior of `package-locales.sh`:

| Variable | Required | Example | Description |
|----------|----------|---------|-------------|
| `ROOT_DIR` | Yes | `/path/to/vitte` | Vitte repository root |
| `STAGE_ROOT` | Yes | `$root/.debstage/vitte/root` | Staging directory |
| `TARGET_LOCALE_DIR` | No | `/usr/share/vitte/locales` | Installation path for locales |
| `INSTALL_TYPE` | No | `debian` or `macos` | Installer type |

## File Locations After Integration

### On Debian/Linux
```
/usr/share/vitte/
  ├── locales/              # Locale files
  │   ├── en/
  │   ├── fr/
  │   ├── de/
  │   └── ...
  ├── scripts/
  │   └── configure-language.sh
  └── ...

/usr/bin/vitte-configure-language  # Symlink for easy access
/etc/vitte/language.conf            # User's language preference
```

### On macOS
```
/usr/local/vitte/
  ├── locales/              # Locale files
  │   ├── en/
  │   ├── fr/
  │   ├── de/
  │   └── ...
  ├── scripts/
  │   └── configure-language.sh
  └── ...

/usr/local/bin/vitte-configure-language  # Symlink for easy access
/etc/vitte/language.conf                  # User's language preference
```

## Testing the Integration

### Test Debian build

```bash
# Build the package
make -f Makefile.deb deb

# Extract and verify locales are present
cd /tmp
dpkg -x pkgout/vitte_*.deb vitte-test
ls -la vitte-test/usr/share/vitte/locales/
```

### Test macOS build

```bash
# Build the package
make -f Makefile.pkg pkg

# Open and inspect the installer
open pkgout/Vitte-*.pkg

# After installation, verify
ls -la /usr/local/vitte/locales/
cat /etc/vitte/language.conf
```

## Troubleshooting

### Locales not included in package

1. Check source locales exist:
   ```bash
   ls -la locales/
   ```

2. Verify staging directory:
   ```bash
   echo $STAGE_ROOT
   ls -la $STAGE_ROOT/usr/share/vitte/locales/
   ```

3. Run packaging manually:
   ```bash
   export ROOT_DIR=$(pwd)
   export STAGE_ROOT=/tmp/test-stage
   mkdir -p $STAGE_ROOT
   bash scripts/package-locales.sh
   ```

### debconf not being used

Verify Debian control file includes `debconf` dependency:
```bash
grep -i debconf .debstage/*/DEBIAN/control
```

### Language selection not working after installation

1. Check configuration file:
   ```bash
   cat /etc/vitte/language.conf
   ```

2. Verify locale files are installed:
   ```bash
   ls /usr/share/vitte/locales/fr/
   ```

3. Test manual configuration:
   ```bash
   sudo /usr/bin/vitte-configure-language
   ```

## Performance Considerations

- Locale files are pre-installed (not downloaded during installation)
- User only selects preference, not full downloads
- Future: Optional lazy-loading for less common languages
- Consider: Separate language packs for minimal installations

## Security Considerations

- Locale files are read-only (644 permissions)
- Configuration file is readable by all (644 permissions)
- Language selector requires sudo for system-wide changes
- No injection risks from locale file loading (FTL is static)

## Future Enhancements

1. **Lazy Loading**: Download language packs on-demand
2. **Auto-Detection**: Detect system locale and set automatically
3. **UI Integration**: GUI language selector for non-technical users
4. **Cloud Delivery**: CDN for large language packs
5. **Versioning**: Track locale version separately from main package

## Support

For issues or improvements:

1. Check LANGUAGE_SELECTION.md for user documentation
2. Review build script integration
3. Verify locale files are valid .ftl format
4. Test on target platform (Debian, macOS, etc.)

---

**Last Updated:** 2026-06-23
**Status:** Reference Implementation
**Maintainer:** Vitte Build System Team
