#!/usr/bin/env bash
set -Eeuo pipefail

# ============================================================================
# Vitte language support installer for Geany
#
# Installs:
#   - filetypes.vitte.conf
#   - filetypes.Vitte.conf
#   - Vitte file-extension mappings
#   - Vitte snippets
#
# Environment:
#   GEANY_HOME               Override the Geany configuration directory.
#   VITTE_GEANY_WD_MODE      file | project | current
#   VITTE_GEANY_DRY_RUN      1 to print operations without modifying files.
#   VITTE_GEANY_BACKUP       0 to disable backups.
#   VITTE_GEANY_UNINSTALL    1 to remove Vitte integration.
# ============================================================================

readonly SCRIPT_NAME="${0##*/}"
readonly SCRIPT_DIR="$(
  cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
  pwd -P
)"

readonly FILETYPE_SOURCE="$SCRIPT_DIR/filetypes.vitte.conf"
readonly SNIPPETS_SOURCE="$SCRIPT_DIR/snippets.vitte.conf"

readonly SNIPPET_BEGIN="# BEGIN Vitte snippets"
readonly SNIPPET_END="# END Vitte snippets"

readonly EXTENSION_LINE="vitte=*.vit;*.vitte;*.vitl;"

DRY_RUN="${VITTE_GEANY_DRY_RUN:-0}"
BACKUP_ENABLED="${VITTE_GEANY_BACKUP:-1}"
UNINSTALL_MODE="${VITTE_GEANY_UNINSTALL:-0}"

TEMP_PATHS=()

log() {
  printf '[geany] %s\n' "$*"
}

warn() {
  printf '[geany][warning] %s\n' "$*" >&2
}

error() {
  printf '[geany][error] %s\n' "$*" >&2
}

die() {
  error "$*"
  exit 1
}

cleanup() {
  local path

  for path in "${TEMP_PATHS[@]:-}"; do
    if [[ -n "$path" && -e "$path" ]]; then
      rm -f -- "$path"
    fi
  done
}

on_error() {
  local exit_code=$?
  local line_number="${1:-unknown}"

  error "installation failed at line $line_number with status $exit_code"
  exit "$exit_code"
}

trap cleanup EXIT
trap 'on_error "$LINENO"' ERR

is_enabled() {
  case "${1:-0}" in
    1|true|TRUE|yes|YES|on|ON)
      return 0
      ;;
    *)
      return 1
      ;;
  esac
}

run() {
  if is_enabled "$DRY_RUN"; then
    printf '[geany][dry-run]'
    printf ' %q' "$@"
    printf '\n'
    return 0
  fi

  "$@"
}

make_temp_file() {
  local directory="${1:-${TMPDIR:-/tmp}}"
  local template="${2:-vitte-geany.XXXXXX}"
  local temp_file

  mkdir -p -- "$directory"
  temp_file="$(mktemp "$directory/$template")"
  TEMP_PATHS+=("$temp_file")

  printf '%s\n' "$temp_file"
}

require_file() {
  local path="$1"

  [[ -f "$path" ]] || die "required file not found: $path"
  [[ -r "$path" ]] || die "required file is not readable: $path"
}

backup_file() {
  local path="$1"
  local timestamp
  local backup

  is_enabled "$BACKUP_ENABLED" || return 0
  [[ -f "$path" ]] || return 0

  timestamp="$(date '+%Y%m%d-%H%M%S')"
  backup="${path}.bak.${timestamp}"

  run cp -p -- "$path" "$backup"
  log "backup: $backup"
}

atomic_install() {
  local source="$1"
  local destination="$2"
  local destination_dir
  local temporary

  destination_dir="$(dirname -- "$destination")"

  run mkdir -p -- "$destination_dir"

  if is_enabled "$DRY_RUN"; then
    run install -m 0644 -- "$source" "$destination"
    return
  fi

  temporary="$(make_temp_file "$destination_dir" '.vitte-install.XXXXXX')"
  install -m 0644 -- "$source" "$temporary"
  mv -f -- "$temporary" "$destination"
}

configure_working_directory() {
  local mode="${VITTE_GEANY_WD_MODE:-file}"

  case "$mode" in
    file)
      WD_MODE="file"
      WD_TOKEN="%d"
      ;;
    project)
      WD_MODE="project"
      WD_TOKEN="%p"
      ;;
    current)
      WD_MODE="current"
      WD_TOKEN="%c"
      ;;
    *)
      warn "unknown VITTE_GEANY_WD_MODE=$mode; expected file, project or current"
      warn "falling back to file-directory mode"
      WD_MODE="file"
      WD_TOKEN="%d"
      ;;
  esac
}

print_unique_paths() {
  awk 'NF && !seen[$0]++'
}

detect_geany_homes() {
  local system
  local xdg_home
  local mac_application_support
  local mac_xdg_home
  local windows_home

  if [[ -n "${GEANY_HOME:-}" ]]; then
    printf '%s\n' "$GEANY_HOME"
    return
  fi

  system="$(uname -s 2>/dev/null || printf 'unknown')"
  xdg_home="${XDG_CONFIG_HOME:-$HOME/.config}/geany"
  mac_application_support="$HOME/Library/Application Support/geany"
  mac_xdg_home="$HOME/.config/geany"

  case "$system" in
    Darwin)
      if [[ -d "$mac_xdg_home" ]]; then
        printf '%s\n' "$mac_xdg_home"
      fi

      if [[ -d "$mac_application_support" ]]; then
        printf '%s\n' "$mac_application_support"
      fi

      if [[ ! -d "$mac_xdg_home" && ! -d "$mac_application_support" ]]; then
        printf '%s\n' "$mac_xdg_home"
      fi
      ;;

    MINGW*|MSYS*|CYGWIN*)
      if [[ -n "${APPDATA:-}" ]]; then
        windows_home="$APPDATA/geany"
      else
        windows_home="$HOME/AppData/Roaming/geany"
      fi

      printf '%s\n' "$windows_home"
      ;;

    *)
      printf '%s\n' "$xdg_home"
      ;;
  esac
}

generate_filetype_definition() {
  local destination="$1"
  local temporary

  temporary="$(make_temp_file)"

  awk -v wd="$WD_TOKEN" '
    /^(FT|EX)_[0-9][0-9]_WD=/ {
      sub(/=.*/, "=" wd)
    }

    {
      print
    }
  ' "$FILETYPE_SOURCE" >"$temporary"

  atomic_install "$temporary" "$destination"
  run rm -f -- "$temporary"
}

ensure_extension_file() {
  local extension_file="$1"
  local temporary

  if [[ ! -f "$extension_file" ]]; then
    if is_enabled "$DRY_RUN"; then
      log "would create: $extension_file"
      return
    fi

    mkdir -p -- "$(dirname -- "$extension_file")"

    cat >"$extension_file" <<'EOF'
[Extensions]
EOF
  fi

  backup_file "$extension_file"

  temporary="$(
    make_temp_file \
      "$(dirname -- "$extension_file")" \
      '.extensions.XXXXXX'
  )"

  awk -v mapping="$EXTENSION_LINE" '
    BEGIN {
      in_extensions = 0
      section_found = 0
    }

    /^[[:space:]]*\[Extensions\][[:space:]]*$/ {
      print
      print mapping

      in_extensions = 1
      section_found = 1
      next
    }

    /^[[:space:]]*\[/ {
      in_extensions = 0
      print
      next
    }

    {
      if (in_extensions && $0 ~ /^[[:space:]]*(vitte|Vitte)[[:space:]]*=/) {
        next
      }

      print
    }

    END {
      if (!section_found) {
        print ""
        print "[Extensions]"
        print mapping
      }
    }
  ' "$extension_file" >"$temporary"

  atomic_install "$temporary" "$extension_file"
  run rm -f -- "$temporary"
}

create_default_snippets_file() {
  local snippets_file="$1"

  if [[ -f "$snippets_file" ]]; then
    return
  fi

  if is_enabled "$DRY_RUN"; then
    log "would create: $snippets_file"
    return
  fi

  mkdir -p -- "$(dirname -- "$snippets_file")"

  cat >"$snippets_file" <<'EOF'
[Default]

[Special]
brace_open=\n{\n\t
brace_close=}\n
block=\n{\n\t%cursor%\n}
block_cursor=\n{\n\t%cursor%\n}
EOF
}

strip_vitte_snippets() {
  local source="$1"
  local destination="$2"

  awk \
    -v begin_marker="$SNIPPET_BEGIN" \
    -v end_marker="$SNIPPET_END" '
    BEGIN {
      in_generated_block = 0
      in_vitte_section = 0
    }

    $0 == begin_marker {
      in_generated_block = 1
      next
    }

    $0 == end_marker {
      in_generated_block = 0
      next
    }

    in_generated_block {
      next
    }

    /^[[:space:]]*\[(vitte|Vitte)\][[:space:]]*$/ {
      in_vitte_section = 1
      next
    }

    /^[[:space:]]*\[/ {
      in_vitte_section = 0
      print
      next
    }

    !in_vitte_section {
      print
    }
  ' "$source" >"$destination"
}

install_snippets() {
  local snippets_file="$1"
  local stripped
  local generated

  create_default_snippets_file "$snippets_file"

  if is_enabled "$DRY_RUN" && [[ ! -f "$snippets_file" ]]; then
    log "would append Vitte snippets to: $snippets_file"
    return
  fi

  backup_file "$snippets_file"

  stripped="$(
    make_temp_file \
      "$(dirname -- "$snippets_file")" \
      '.snippets-stripped.XXXXXX'
  )"

  generated="$(
    make_temp_file \
      "$(dirname -- "$snippets_file")" \
      '.snippets-generated.XXXXXX'
  )"

  strip_vitte_snippets "$snippets_file" "$stripped"

  {
    cat "$stripped"

    if [[ -s "$stripped" ]]; then
      printf '\n'
    fi

    printf '%s\n' "$SNIPPET_BEGIN"
    cat "$SNIPPETS_SOURCE"

    if [[ -s "$SNIPPETS_SOURCE" ]]; then
      last_character="$(tail -c 1 "$SNIPPETS_SOURCE" 2>/dev/null || true)"

      if [[ -n "$last_character" ]]; then
        printf '\n'
      fi
    fi

    printf '%s\n' "$SNIPPET_END"
  } >"$generated"

  atomic_install "$generated" "$snippets_file"
  run rm -f -- "$stripped" "$generated"
}

remove_extension_mapping() {
  local extension_file="$1"
  local temporary

  [[ -f "$extension_file" ]] || return 0

  backup_file "$extension_file"

  temporary="$(
    make_temp_file \
      "$(dirname -- "$extension_file")" \
      '.extensions-remove.XXXXXX'
  )"

  awk '
    BEGIN {
      in_extensions = 0
    }

    /^[[:space:]]*\[Extensions\][[:space:]]*$/ {
      in_extensions = 1
      print
      next
    }

    /^[[:space:]]*\[/ {
      in_extensions = 0
      print
      next
    }

    {
      if (in_extensions && $0 ~ /^[[:space:]]*(vitte|Vitte)[[:space:]]*=/) {
        next
      }

      print
    }
  ' "$extension_file" >"$temporary"

  atomic_install "$temporary" "$extension_file"
  run rm -f -- "$temporary"
}

remove_snippets() {
  local snippets_file="$1"
  local temporary

  [[ -f "$snippets_file" ]] || return 0

  backup_file "$snippets_file"

  temporary="$(
    make_temp_file \
      "$(dirname -- "$snippets_file")" \
      '.snippets-remove.XXXXXX'
  )"

  strip_vitte_snippets "$snippets_file" "$temporary"
  atomic_install "$temporary" "$snippets_file"
  run rm -f -- "$temporary"
}

uninstall_from_geany_home() {
  local geany_home="$1"
  local filedefs_dir="$geany_home/filedefs"
  local snippets_file="$geany_home/snippets.conf"
  local extension_file="$geany_home/filetype_extensions.conf"

  log "removing Vitte support from: $geany_home"

  run rm -f -- \
    "$filedefs_dir/filetypes.vitte.conf" \
    "$filedefs_dir/filetypes.Vitte.conf"

  remove_extension_mapping "$extension_file"
  remove_snippets "$snippets_file"

  log "removed:"
  log "  $filedefs_dir/filetypes.vitte.conf"
  log "  $filedefs_dir/filetypes.Vitte.conf"
  log "  Vitte extension mappings"
  log "  Vitte snippet sections"
}

install_into_geany_home() {
  local geany_home="$1"
  local filedefs_dir="$geany_home/filedefs"
  local snippets_file="$geany_home/snippets.conf"
  local extension_file="$geany_home/filetype_extensions.conf"
  local lowercase_filetype="$filedefs_dir/filetypes.vitte.conf"
  local display_filetype="$filedefs_dir/filetypes.Vitte.conf"

  log "installing Vitte support into: $geany_home"

  run mkdir -p -- "$filedefs_dir"

  backup_file "$lowercase_filetype"
  backup_file "$display_filetype"

  generate_filetype_definition "$lowercase_filetype"
  generate_filetype_definition "$display_filetype"

  ensure_extension_file "$extension_file"
  install_snippets "$snippets_file"

  log "installed:"
  log "  $lowercase_filetype"
  log "  $display_filetype"
  log "  $extension_file"
  log "  $snippets_file"
  log "working-directory mode: $WD_MODE ($WD_TOKEN)"
}

validate_environment() {
  [[ -n "${HOME:-}" ]] || die "HOME is not defined"

  if ! is_enabled "$UNINSTALL_MODE"; then
    require_file "$FILETYPE_SOURCE"
    require_file "$SNIPPETS_SOURCE"
  fi

  command -v awk >/dev/null 2>&1 ||
    die "awk is required"

  command -v install >/dev/null 2>&1 ||
    die "install is required"

  command -v mktemp >/dev/null 2>&1 ||
    die "mktemp is required"

  if ! command -v geany >/dev/null 2>&1; then
    warn "Geany was not found in PATH"
    warn "configuration files will still be installed"
  fi
}

main() {
  local geany_home
  local installed_count=0

  configure_working_directory
  validate_environment

  while IFS= read -r geany_home; do
    [[ -n "$geany_home" ]] || continue

    if is_enabled "$UNINSTALL_MODE"; then
      uninstall_from_geany_home "$geany_home"
    else
      install_into_geany_home "$geany_home"
    fi

    installed_count=$((installed_count + 1))
  done < <(detect_geany_homes | print_unique_paths)

  if (( installed_count == 0 )); then
    die "no Geany configuration directory could be determined"
  fi

  printf '\n'

  if is_enabled "$DRY_RUN"; then
    log "simulation completed; no files were modified"
  elif is_enabled "$UNINSTALL_MODE"; then
    log "Vitte support removed"
    log "restart Geany to reload filetype definitions"
  else
    log "Vitte support installed successfully"
    log "restart Geany to load the new filetype definitions"
  fi
}

main "$@"
