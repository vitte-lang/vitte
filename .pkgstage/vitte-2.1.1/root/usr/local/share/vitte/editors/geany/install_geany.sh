#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

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

if ! command -v geany >/dev/null 2>&1; then
  echo "[geany][warning] geany binary not found in PATH; only config files were installed."
fi

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

install_into_geany_home() {
  local geany_home="$1"
  local filedefs_dir="$geany_home/filedefs"
  local snippets_file="$geany_home/snippets.conf"
  local ext_file="$geany_home/filetype_extensions.conf"

  mkdir -p "$filedefs_dir"

  local generated_filetype
  generated_filetype="$(mktemp)"
  awk -v wd="$wd_token" '
    { if ($0 ~ /^(FT|EX)_[0-9][0-9]_WD=/) { sub(/=.*/, "=" wd, $0) } print $0 }
  ' "$SCRIPT_DIR/filetypes.vitte.conf" >"$generated_filetype"
  install -m 0644 "$generated_filetype" "$filedefs_dir/filetypes.vitte.conf"
  install -m 0644 "$generated_filetype" "$filedefs_dir/filetypes.Vitte.conf"
  rm -f "$generated_filetype"

  if [[ ! -f "$ext_file" ]]; then
    cat >"$ext_file" <<'EOT'
[Extensions]
EOT
  fi

  awk '
    BEGIN { inext=0; inserted=0 }
    /^\[Extensions\]/ {
      print $0
      print "vitte=*.vit;*.vitte;*.vitl;"
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
        print "vitte=*.vit;*.vitte;*.vitl;"
      }
    }
  ' "$ext_file" >"$ext_file.tmp"
  mv "$ext_file.tmp" "$ext_file"

  if [[ ! -f "$snippets_file" ]]; then
    cat >"$snippets_file" <<'EOT'
[Default]

[Special]
brace_open=\n{\n\t
brace_close=}\n
block=\n{\n\t%cursor%\n}
block_cursor=\n{\n\t%cursor%\n}
EOT
  fi

  awk '
    BEGIN { skip=0; marker=0 }
    /^# BEGIN Vitte snippets$/ { marker=1; next }
    /^# END Vitte snippets$/ { marker=0; next }
    marker==1 { next }
    /^\[(vitte|Vitte)\]$/ { skip=1; next }
    /^\[/ {
      if (skip==1) skip=0
      print $0
      next
    }
    { if (skip==0) print $0 }
  ' "$snippets_file" >"$snippets_file.tmp"
  mv "$snippets_file.tmp" "$snippets_file"
  {
    echo
    echo "# BEGIN Vitte snippets"
    cat "$SCRIPT_DIR/snippets.vitte.conf"
    echo "# END Vitte snippets"
  } >>"$snippets_file"

  echo "Geany Vitte config installed:"
  echo "  - $filedefs_dir/filetypes.vitte.conf"
  echo "  - $filedefs_dir/filetypes.Vitte.conf"
  echo "  - $ext_file (mapping *.vit, *.vitte, *.vitl)"
  echo "  - $snippets_file (sections [vitte] and [Vitte])"
  echo "  - WD mode: $wd_mode ($wd_token)"
  echo
}

while IFS= read -r geany_home; do
  [ -n "$geany_home" ] || continue
  install_into_geany_home "$geany_home"
done < <(detect_geany_homes)

echo "Restart Geany."
