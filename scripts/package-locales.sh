#!/usr/bin/env bash
# ============================================================
# package-locales.sh
#
# Helper script to package locale files (.ftl) for distribution
# Copies language files from source locales/ to installer staging
# ============================================================

set -euo pipefail

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Helper functions
log() {
  printf "${BLUE}[package-locales]${NC} %s\n" "$*"
}

success() {
  printf "${GREEN}[package-locales]${NC} ✓ %s\n" "$*"
}

error() {
  printf "${RED}[package-locales][ERROR]${NC} %s\n" "$*" >&2
}

die() {
  error "$@"
  exit 1
}

# Configuration from environment
ROOT_DIR="${ROOT_DIR:-.}"
STAGE_ROOT="${STAGE_ROOT:?STAGE_ROOT not set - specify staging directory}"
TARGET_LOCALE_DIR="${TARGET_LOCALE_DIR:-/usr/share/vitte/locales}"
INSTALL_TYPE="${INSTALL_TYPE:-debian}"  # debian or macos

# Source locales directory
SOURCE_LOCALES_DIR="$ROOT_DIR/locales"

# Available languages
declare -a LANGUAGES=(
  en fr de es it ja ko zh-CN nl pl pt-BR ru tr uk ar
)

# ============================================================
# Main Functions
# ============================================================

# Validate source locales exist
validate_source() {
  if [ ! -d "$SOURCE_LOCALES_DIR" ]; then
    die "Source locales directory not found: $SOURCE_LOCALES_DIR"
  fi

  log "Source locales directory: $SOURCE_LOCALES_DIR"
}

# Create locale directory in staging
create_staging_dir() {
  local stage_locales="$STAGE_ROOT$TARGET_LOCALE_DIR"

  log "Creating staging directory: $stage_locales"
  mkdir -p "$stage_locales"
  chmod 755 "$stage_locales"

  success "Staging directory created"
}

# Copy locale files
copy_locales() {
  local stage_locales="$STAGE_ROOT$TARGET_LOCALE_DIR"
  local copied=0
  local missing=0

  log "Copying locale files for ${#LANGUAGES[@]} languages..."

  for lang in "${LANGUAGES[@]}"; do
    local src_lang_dir="$SOURCE_LOCALES_DIR/$lang"
    local dst_lang_dir="$stage_locales/$lang"

    if [ -d "$src_lang_dir" ]; then
      mkdir -p "$dst_lang_dir"

      # Copy all .ftl files
      if find "$src_lang_dir" -name "*.ftl" -type f 2>/dev/null | grep -q .; then
        rsync -a "$src_lang_dir"/ "$dst_lang_dir/"
        chmod -R 644 "$dst_lang_dir"
        chmod 755 "$dst_lang_dir"

        local count=$(find "$dst_lang_dir" -name "*.ftl" -type f | wc -l)
        printf "  ${GREEN}✓${NC} %-10s %d files\n" "$lang" "$count"
        ((copied++))
      else
        printf "  ${YELLOW}!${NC} %-10s (no .ftl files)\n" "$lang"
        ((missing++))
      fi
    else
      printf "  ${YELLOW}!${NC} %-10s (directory not found)\n" "$lang"
      ((missing++))
    fi
  done

  echo ""
  printf "  Copied:    %d languages\n" "$copied"
  printf "  Missing:   %d languages\n" "$missing"

  if [ "$copied" -eq 0 ]; then
    error "No locales were copied!"
    return 1
  fi

  success "Locale files packaged"
}

# Install configure-language script
install_language_config() {
  local configure_script="$ROOT_DIR/scripts/configure-language.sh"
  local stage_scripts="$STAGE_ROOT/usr/share/vitte/scripts"

  if [ ! -f "$configure_script" ]; then
    error "Language configuration script not found: $configure_script"
    return 1
  fi

  log "Installing language configuration utility..."

  mkdir -p "$stage_scripts"
  install -m 0755 "$configure_script" "$stage_scripts/configure-language.sh"

  # Create symlink in /usr/bin for easy access
  if [ "$INSTALL_TYPE" = "debian" ]; then
    mkdir -p "$STAGE_ROOT/usr/bin"
    ln -sf /usr/share/vitte/scripts/configure-language.sh \
      "$STAGE_ROOT/usr/bin/vitte-configure-language" || true
  fi

  success "Language configuration installed"
}

# Install debconf files (Debian only)
install_debconf_files() {
  if [ "$INSTALL_TYPE" != "debian" ]; then
    return 0
  fi

  local debconf_dir="$ROOT_DIR/toolchain/scripts/package/debian"
  local stage_debian="$STAGE_ROOT/DEBIAN"

  if [ ! -d "$debconf_dir" ]; then
    log "Debconf files not found (optional)"
    return 0
  fi

  log "Installing debconf configuration..."

  # Copy templates
  if [ -f "$debconf_dir/templates" ]; then
    mkdir -p "$stage_debian"
    install -m 0644 "$debconf_dir/templates" "$stage_debian/templates"
    success "Debconf templates installed"
  fi

  # Note: config and postinst are handled separately by build script
}

# Create locale manifest
create_manifest() {
  local stage_locales="$STAGE_ROOT$TARGET_LOCALE_DIR"
  local manifest="$stage_locales/MANIFEST.txt"

  log "Creating locale manifest..."

  {
    echo "# Vitte Locale Manifest"
    echo "# Generated: $(date -u '+%Y-%m-%dT%H:%M:%SZ')"
    echo ""
    echo "Installed Languages:"
    echo ""

    for lang in "${LANGUAGES[@]}"; do
      if [ -d "$stage_locales/$lang" ]; then
        local count=$(find "$stage_locales/$lang" -name "*.ftl" -type f | wc -l)
        printf "  %s: %d files\n" "$lang" "$count"
      fi
    done

    echo ""
    echo "Configuration:"
    echo "  Config File: /etc/vitte/language.conf"
    echo "  Language Dir: $TARGET_LOCALE_DIR"
    echo "  Selector: vitte-configure-language"
  } > "$manifest"

  chmod 644 "$manifest"
  success "Manifest created: $manifest"
}

# Verify installation
verify_installation() {
  log "Verifying locale installation..."

  local stage_locales="$STAGE_ROOT$TARGET_LOCALE_DIR"
  local errors=0

  # Check each language has at least some files
  for lang in "${LANGUAGES[@]}"; do
    if [ -d "$stage_locales/$lang" ]; then
      local count=$(find "$stage_locales/$lang" -name "*.ftl" -type f 2>/dev/null | wc -l)
      if [ "$count" -gt 0 ]; then
        printf "  ${GREEN}✓${NC} %s (%d files)\n" "$lang" "$count"
      else
        printf "  ${RED}✗${NC} %s (0 files)\n" "$lang"
        ((errors++))
      fi
    else
      printf "  ${RED}✗${NC} %s (not found)\n" "$lang"
      ((errors++))
    fi
  done

  if [ "$errors" -gt 0 ]; then
    error "Verification found $errors issues"
    return 1
  fi

  success "All locales verified"
}

# ============================================================
# Main
# ============================================================

main() {
  log "Packaging Vitte Locale Files"
  echo ""

  validate_source
  create_staging_dir
  copy_locales
  install_language_config
  install_debconf_files
  create_manifest

  echo ""
  verify_installation

  echo ""
  success "Locale packaging complete"
}

main "$@"
