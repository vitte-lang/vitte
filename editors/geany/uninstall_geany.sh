#!/usr/bin/env bash
set -euo pipefail

detect_geany_homes() {
  if [[ -n "${GEANY_HOME:-}" ]]; then
    printf "%s\n" "$GEANY_HOME"
    return
  fi

  local xdg_home="${XDG_CONFIG_HOME:-$HOME/.config}/geany"
  local mac_home="$HOME/Library/Application Support/geany"

  case "$(uname -s 2>/dev/null || echo unknown)" in
    Darwin)
      if [[ -d "$xdg_home" ]]; then printf "%s\n" "$xdg_home"; fi
      if [[ -d "$mac_home" ]]; then printf "%s\n" "$mac_home"; fi
      if [[ ! -d "$xdg_home" && ! -d "$mac_home" ]]; then printf "%s\n" "$xdg_home"; fi
      ;;
    MINGW*|MSYS*|CYGWIN*)
      if [[ -n "${APPDATA:-}" ]]; then
        printf "%s\n" "$APPDATA/geany"
      else
        printf "%s\n" "$HOME/AppData/Roaming/geany"
      fi
      ;;
    *)
      printf "%s\n" "$xdg_home"
      ;;
  esac
}

uninstall_from_geany_home() {
  local geany_home="$1"
  local filedefs_dir="$geany_home/filedefs"
  local snippets_file="$geany_home/snippets.conf"
  local ext_file="$geany_home/filetype_extensions.conf"

  rm -f "$filedefs_dir/filetypes.vitte.conf" "$filedefs_dir/filetypes.Vitte.conf"

  if [[ -f "$ext_file" ]]; then
    awk '
      { if ($0 ~ /^(Vitte|vitte)=.*\*\.vit/) next; print $0 }
    ' "$ext_file" >"$ext_file.tmp"
    mv "$ext_file.tmp" "$ext_file"
  fi

  if [[ -f "$snippets_file" ]]; then
    awk '
      BEGIN { skip=0 }
      /^\[(vitte|Vitte)\]$/ { skip=1; next }
      /^\[/ {
        if (skip==1) skip=0
        print $0
        next
      }
      { if (skip==0) print $0 }
    ' "$snippets_file" >"$snippets_file.tmp"
    mv "$snippets_file.tmp" "$snippets_file"
  fi

  echo "Geany Vitte config removed from: $geany_home"
}

while IFS= read -r geany_home; do
  [[ -n "$geany_home" ]] || continue
  uninstall_from_geany_home "$geany_home"
done < <(detect_geany_homes)
