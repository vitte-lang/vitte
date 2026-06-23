#!/usr/bin/env bash
# ============================================================
# configure-language.sh
#
# Interactive language selector for Vitte
# Allows users to choose their preferred language
# Stores selection in /etc/vitte/language.conf
# ============================================================

set -euo pipefail

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Configuration
CONFIG_DIR="/etc/vitte"
CONFIG_FILE="$CONFIG_DIR/language.conf"
LOCALES_DIR="/usr/local/vitte/locales"
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"

# Available languages
declare -A LANGUAGES=(
  [en]="English (English)"
  [fr]="Français (French)"
  [de]="Deutsch (German)"
  [es]="Español (Spanish)"
  [it]="Italiano (Italian)"
  [ja]="日本語 (Japanese)"
  [ko]="한국어 (Korean)"
  [zh-CN]="中文 (Simplified Chinese)"
  [nl]="Nederlands (Dutch)"
  [pl]="Polski (Polish)"
  [pt-BR]="Português Brasileiro (Brazilian Portuguese)"
  [ru]="Русский (Russian)"
  [tr]="Türkçe (Turkish)"
  [uk]="Українська (Ukrainian)"
  [ar]="العربية (Arabic)"
)

# Helper functions
log() {
  printf "${BLUE}[vitte-language]${NC} %s\n" "$*"
}

success() {
  printf "${GREEN}[vitte-language]${NC} ✓ %s\n" "$*"
}

warn() {
  printf "${YELLOW}[vitte-language]${NC} ⚠ %s\n" "$*"
}

error() {
  printf "${RED}[vitte-language][ERROR]${NC} %s\n" "$*" >&2
}

die() {
  error "$@"
  exit 1
}

# Check if running as root for system-wide installation
check_permissions() {
  if [[ "$EUID" -ne 0 ]]; then
    warn "Not running as root - language preference will be saved to user config"
    CONFIG_DIR="${HOME}/.config/vitte"
    CONFIG_FILE="$CONFIG_DIR/language.conf"
  fi
}

# Ensure config directory exists
ensure_config_dir() {
  if [ ! -d "$CONFIG_DIR" ]; then
    mkdir -p "$CONFIG_DIR"
    chmod 755 "$CONFIG_DIR"
  fi
}

# Display language options
show_menu() {
  local index=1
  local -a lang_codes

  echo ""
  echo -e "${BLUE}Available Languages / Langues disponibles${NC}"
  echo "=========================================="
  echo ""

  # Show first 8 languages in two columns
  for lang_code in "${!LANGUAGES[@]}"; do
    lang_codes+=("$lang_code")
  done

  # Sort for consistent display
  IFS=$'\n' sorted=($(sort <<<"${LANGUAGES[*]}")); unset IFS

  for lang_code in $(printf '%s\n' "${!LANGUAGES[@]}" | sort); do
    printf "%2d) %s\n" "$index" "${LANGUAGES[$lang_code]}"
    ((index++))
  done

  echo ""
  echo -e "${YELLOW}Default: 1 (English)${NC}"
  echo ""
}

# Get user selection
get_selection() {
  local input
  local -a lang_array=()

  # Build sorted array of language codes
  for lang_code in $(printf '%s\n' "${!LANGUAGES[@]}" | sort); do
    lang_array+=("$lang_code")
  done

  read -p "Select language number (1-${#LANGUAGES[@]}): " input

  # Default to English (1) if empty
  if [[ -z "$input" ]]; then
    input=1
  fi

  # Validate input
  if ! [[ "$input" =~ ^[0-9]+$ ]] || [ "$input" -lt 1 ] || [ "$input" -gt "${#LANGUAGES[@]}" ]; then
    error "Invalid selection: $input"
    return 1
  fi

  # Get the language code (adjust for 0-indexing)
  selected_lang="${lang_array[$((input - 1))]}"
  echo "$selected_lang"
}

# Verify selected language has locale files
verify_language() {
  local lang="$1"
  local locale_path

  # Check if locales exist
  if [ ! -d "$LOCALES_DIR/$lang" ] && [ ! -d "$ROOT_DIR/locales/$lang" ]; then
    error "Language files not found for: $lang"
    return 1
  fi

  return 0
}

# Save language selection
save_language() {
  local lang="$1"

  ensure_config_dir

  # Write configuration
  {
    echo "# Vitte Language Configuration"
    echo "# Generated: $(date -u '+%Y-%m-%dT%H:%M:%SZ')"
    echo "# Language: $lang (${LANGUAGES[$lang]})"
    echo ""
    echo "LANGUAGE=$lang"
    echo "VITTE_LOCALE_DIR=$LOCALES_DIR"
  } > "$CONFIG_FILE"

  chmod 644 "$CONFIG_FILE"
  success "Language saved to: $CONFIG_FILE"
}

# Display summary
show_summary() {
  local lang="$1"

  echo ""
  echo -e "${GREEN}Configuration Summary${NC}"
  echo "===================="
  echo "Language:       ${LANGUAGES[$lang]}"
  echo "Language Code:  $lang"
  echo "Config File:    $CONFIG_FILE"
  echo ""
  echo -e "${YELLOW}Note:${NC} Vitte and its tools will use this language"
  echo "      for all future interactions."
  echo ""
}

# Interactive menu
interactive_menu() {
  while true; do
    show_menu

    selected_lang=$(get_selection) || continue

    if verify_language "$selected_lang"; then
      echo ""
      log "Selected: ${LANGUAGES[$selected_lang]}"

      read -p "Confirm selection? (y/n): " confirm
      if [[ "$confirm" =~ ^[Yy]$ ]]; then
        save_language "$selected_lang"
        show_summary "$selected_lang"
        return 0
      fi
    fi
  done
}

# Main
main() {
  check_permissions

  log "Vitte Language Configuration Tool"
  echo ""

  # Check if language already configured
  if [ -f "$CONFIG_FILE" ]; then
    current_lang=$(grep "^LANGUAGE=" "$CONFIG_FILE" | cut -d'=' -f2)
    warn "Current language: ${LANGUAGES[$current_lang]}"

    read -p "Change language? (y/n): " change
    if ! [[ "$change" =~ ^[Yy]$ ]]; then
      success "Language unchanged"
      exit 0
    fi
  fi

  interactive_menu
}

main "$@"
