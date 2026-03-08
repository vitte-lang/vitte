#!/usr/bin/env bash
set -euo pipefail

detect_geany_home() {
  if [[ -n "${GEANY_HOME:-}" ]]; then
    printf "%s\n" "$GEANY_HOME"
    return
  fi

  case "$(uname -s 2>/dev/null || echo unknown)" in
    Darwin)
      printf "%s\n" "$HOME/Library/Application Support/geany"
      ;;
    MINGW*|MSYS*|CYGWIN*)
      if [[ -n "${APPDATA:-}" ]]; then
        printf "%s\n" "$APPDATA/geany"
      else
        printf "%s\n" "$HOME/AppData/Roaming/geany"
      fi
      ;;
    *)
      printf "%s\n" "${XDG_CONFIG_HOME:-$HOME/.config}/geany"
      ;;
  esac
}

GEANY_HOME="$(detect_geany_home)"
FILEDEFS_DIR="$GEANY_HOME/filedefs"
SNIPPETS_FILE="$GEANY_HOME/snippets.conf"
EXT_FILE="$GEANY_HOME/filetype_extensions.conf"

rm -f "$FILEDEFS_DIR/filetypes.vitte.conf"

if [[ -f "$EXT_FILE" ]]; then
  awk '
    { if ($0 ~ /^(Vitte|vitte)=.*\*\.vit/) next; print $0 }
  ' "$EXT_FILE" >"$EXT_FILE.tmp"
  mv "$EXT_FILE.tmp" "$EXT_FILE"
fi

if [[ -f "$SNIPPETS_FILE" ]]; then
  awk '
    BEGIN { skip=0 }
    /^\[(vitte|Vitte)\]$/ { skip=1; next }
    /^\[/ {
      if (skip==1) skip=0
      print $0
      next
    }
    { if (skip==0) print $0 }
  ' "$SNIPPETS_FILE" >"$SNIPPETS_FILE.tmp"
  mv "$SNIPPETS_FILE.tmp" "$SNIPPETS_FILE"
fi

echo "Geany Vitte config removed from: $GEANY_HOME"
