#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GEANY_HOME="${XDG_CONFIG_HOME:-$HOME/.config}/geany"
FILEDEFS_DIR="$GEANY_HOME/filedefs"
SNIPPETS_FILE="$GEANY_HOME/snippets.conf"
EXT_FILE="$GEANY_HOME/filetype_extensions.conf"

mkdir -p "$FILEDEFS_DIR"

cp "$SCRIPT_DIR/filetypes.vitte.conf" "$FILEDEFS_DIR/filetypes.vitte.conf"

if [[ ! -f "$EXT_FILE" ]]; then
  cat >"$EXT_FILE" <<'EOF'
[Extensions]
EOF
fi

if ! grep -Eq '^Vitte=.*\*\.vit' "$EXT_FILE" && ! grep -Eq '^vitte=.*\*\.vit' "$EXT_FILE"; then
  if grep -q '^\[Extensions\]' "$EXT_FILE"; then
    awk '
      BEGIN { inserted=0 }
      /^\[Extensions\]/ {
        print $0
        print "Vitte=*.vit;"
        inserted=1
        next
      }
      { print $0 }
      END {
        if (inserted==0) {
          print "[Extensions]"
          print "Vitte=*.vit;"
        }
      }
    ' "$EXT_FILE" >"$EXT_FILE.tmp"
    mv "$EXT_FILE.tmp" "$EXT_FILE"
  else
    {
      echo
      echo "[Extensions]"
      echo "Vitte=*.vit;"
    } >>"$EXT_FILE"
  fi
fi

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

if ! grep -Eq '^\[(vitte|Vitte)\]$' "$SNIPPETS_FILE"; then
  {
    echo
    cat "$SCRIPT_DIR/snippets.vitte.conf"
  } >>"$SNIPPETS_FILE"
fi

echo "Geany Vitte config installed:"
echo "  - $FILEDEFS_DIR/filetypes.vitte.conf"
echo "  - $EXT_FILE (mapping *.vit)"
echo "  - $SNIPPETS_FILE (section [vitte])"
echo
echo "Restart Geany."
