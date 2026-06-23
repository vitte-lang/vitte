# Vitte Language Selection System

## Overview

This document describes the new language selection feature added to Vitte installers for both macOS and Debian/Linux systems.

## Available Languages

- English (en) - Default
- Français (fr)
- Deutsch (de)
- Español (es)
- Italiano (it)
- 日本語 (ja)
- 한국어 (ko)
- 中文 (zh-CN) - Simplified Chinese
- Nederlands (nl)
- Polski (pl)
- Português Brasileiro (pt-BR)
- Русский (ru)
- Türkçe (tr)
- Українська (uk)
- العربية (ar)

## Installation and Language Selection

### macOS (.pkg Installer)

During installation on macOS:

1. Open the Vitte .pkg installer
2. Navigate to the "Installation Type" or "Customize" screen
3. Select desired languages (English is selected by default)
4. Complete the installation

**After Installation:**
- Language files are installed to `/usr/local/vitte/locales/`
- Language preference is stored in `/etc/vitte/language.conf`
- To change language later:
  ```bash
  sudo vitte-configure-language
  ```

### Debian/Linux (.deb Installer)

During installation on Debian/Linux:

1. Install the package:
   ```bash
   sudo dpkg -i vitte_*.deb
   ```
2. When prompted by debconf, select your preferred language
3. The package will configure your language preference automatically

**After Installation:**
- Language files are installed to `/usr/share/vitte/locales/`
- Language preference is stored in `/etc/vitte/language.conf`
- To change language later:
  ```bash
  sudo vitte-configure-language
  ```

## Configuration Files

### /etc/vitte/language.conf

Example content:
```bash
# Vitte Language Configuration
# Generated: 2026-06-23T23:08:00Z

LANGUAGE=fr
VITTE_LOCALE_DIR=/usr/share/vitte/locales
```

This file is automatically created during installation with the selected language.

## Language Selection Utility

### vitte-configure-language

An interactive shell script to select or change the language:

```bash
# System-wide configuration (requires sudo)
sudo vitte-configure-language

# User-specific configuration (no sudo needed, config goes to ~/.config/vitte/)
vitte-configure-language
```

**Features:**
- Interactive menu with all available languages
- Displays language names in both native and English
- Confirms selection before saving
- Stores configuration in appropriate location (system or user home)

### Manual Configuration

You can manually edit the language configuration:

```bash
# View current setting
cat /etc/vitte/language.conf

# Change to French
echo "LANGUAGE=fr" | sudo tee /etc/vitte/language.conf
```

## How Vitte Uses Language Settings

When Vitte or its tools run:

1. They read `/etc/vitte/language.conf` (or `~/.config/vitte/language.conf`)
2. Extract the `LANGUAGE` variable
3. Load locale files from `$VITTE_LOCALE_DIR/$LANGUAGE/*.ftl`
4. Display messages, help text, and documentation in the selected language
5. Default to English if language files not found or config not set

## Files Modified/Added for This Feature

### New Files

1. **scripts/configure-language.sh**
   - Interactive language selection utility
   - Used both as a standalone command and in installers

2. **scripts/installer/Distribution.xml**
   - macOS installer configuration
   - Defines language choices and components
   - Used by `productbuild` to create the final .pkg

3. **toolchain/scripts/package/debian/config**
   - Debconf configuration script
   - Prompts user for language selection during Debian installation

4. **toolchain/scripts/package/debian/postinst**
   - Debian postinstall script
   - Saves language preference to /etc/vitte/language.conf
   - Installs language configuration utility

5. **toolchain/scripts/package/debian/templates**
   - Debconf templates for language selection UI
   - Defines language choices shown to user

### Modified Files

1. **Makefile.deb** (and variants)
   - Add debconf dependency
   - Include language configuration scripts in package

2. **scripts/build-*.sh** (macOS and Debian)
   - Copy language configuration files to package staging
   - Update postinstall scripts to set default language
   - Integrate Distribution.xml for macOS

## Testing Language Selection

### Test Debian Installation

```bash
# Build a test .deb package
make -f Makefile.deb deb

# Install it (will prompt for language)
sudo dpkg -i pkgout/vitte_*.deb

# Verify language was set
cat /etc/vitte/language.conf

# Test language selection tool
sudo vitte-configure-language
```

### Test macOS Installation

```bash
# Build a test .pkg package
make -f Makefile.pkg pkg

# Open the installer and test language selection
open pkgout/Vitte-*.pkg

# Verify after installation
cat /etc/vitte/language.conf
sudo vitte-configure-language
```

## Troubleshooting

### Language not changing

1. Check if language files exist:
   ```bash
   ls /usr/share/vitte/locales/  # Debian
   ls /usr/local/vitte/locales/  # macOS
   ```

2. Verify configuration file:
   ```bash
   cat /etc/vitte/language.conf
   ```

3. Try manual configuration:
   ```bash
   echo "LANGUAGE=en" | sudo tee /etc/vitte/language.conf
   ```

### debconf not prompting

If debconf prompts are not showing:

```bash
# Manually run language configuration
sudo /usr/bin/vitte-configure-language

# Or reconfigure the package
sudo dpkg-reconfigure vitte
```

## Environment Variables

For Vitte tools, you can also override language via environment variable:

```bash
# Use French for this command only
LANGUAGE=fr vitte --help

# Or set it in your shell profile
export LANGUAGE=fr
```

Note: The configuration file takes precedence over any system locale settings.

## Future Enhancements

Potential improvements to this system:

1. GUI language selector for macOS (beyond Installer.app)
2. Web-based installer with language selection for cross-platform
3. Automatic language detection from system locale
4. Per-user language preferences without requiring sudo
5. Language packs as optional download components
6. Translation quality scoring and community contributions

## Support

For issues or suggestions regarding language support:

- Check available translations: `ls locales/`
- Submit translations: Contribute to translation team
- Report missing translations: File an issue with language code
- Request new languages: Submit language support request

---

**Last Updated:** 2026-06-23
**Status:** Active
**Maintainer:** Vitte Language Team
