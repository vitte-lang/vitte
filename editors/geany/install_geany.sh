#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
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

mkdir -p "$FILEDEFS_DIR"

cp "$SCRIPT_DIR/filetypes.vitte.conf" "$FILEDEFS_DIR/filetypes.vitte.conf"
wd_mode="${VITTE_GEANY_WD_MODE:-file}"
case "$wd_mode" in
  file) wd_token="%d" ;;
  project) wd_token="%p" ;;
  current) wd_token="%c" ;;
  *)
    echo "[geany][warning] unknown VITTE_GEANY_WD_MODE=$wd_mode (expected file|project|current); using file"
    wd_token="%d"
    wd_mode="file"
    ;;
esac
awk -v wd="$wd_token" '
  { if ($0 ~ /^(FT|EX)_[0-9][0-9]_WD=/) { sub(/=.*/, "=" wd, $0) } print $0 }
' "$FILEDEFS_DIR/filetypes.vitte.conf" >"$FILEDEFS_DIR/filetypes.vitte.conf.tmp"
mv "$FILEDEFS_DIR/filetypes.vitte.conf.tmp" "$FILEDEFS_DIR/filetypes.vitte.conf"

if ! command -v geany >/dev/null 2>&1; then
  echo "[geany][warning] geany binary not found in PATH; only config files were installed."
fi

if [[ ! -f "$EXT_FILE" ]]; then
  cat >"$EXT_FILE" <<'EOF'
[Extensions]
EOF
fi

# Idempotent strict: keep a single Vitte=*.vit mapping in [Extensions].
awk '
  BEGIN { inext=0; inserted=0 }
  /^\[Extensions\]/ {
    print $0
    print "Vitte=*.vit;"
    inext=1
    inserted=1
    next
  }
  /^\[/ {
    inext=0
    print $0
    next
  }
  {
    if (inext && $0 ~ /^(Vitte|vitte)=.*\*\.vit/) next
    print $0
  }
  END {
    if (inserted==0) {
      print "[Extensions]"
      print "Vitte=*.vit;"
    }
  }
' "$EXT_FILE" >"$EXT_FILE.tmp"
mv "$EXT_FILE.tmp" "$EXT_FILE"

if [[ ! -f "$SNIPPETS_FILE" ]]; then
  cat >"$SNIPPETS_FILE" <<'EOF'
[Default]

[Special]
brace_open=\n{\n\t
brace_close=}\n
block=\n{\n\t%cursor%\n}
block_cursor=\n{\n\t%cursor%\n}
EOF
fi

# Idempotent strict: replace existing [vitte]/[Vitte] blocks, then append canonical.
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
{
  echo
  cat "$SCRIPT_DIR/snippets.vitte.conf"
} >>"$SNIPPETS_FILE"

echo "Geany Vitte config installed:"
echo "  - $FILEDEFS_DIR/filetypes.vitte.conf"
echo "  - $EXT_FILE (mapping *.vit)"
echo "  - $SNIPPETS_FILE (section [vitte])"
echo "  - WD mode: $wd_mode ($wd_token)"
echo
echo "Restart Geany."
