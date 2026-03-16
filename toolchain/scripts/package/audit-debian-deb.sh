#!/usr/bin/env bash
set -eu

if [ "$#" -ne 1 ]; then
  echo "usage: $0 <path-to-deb>" >&2
  exit 2
fi

DEB_PATH="$1"
[ -f "$DEB_PATH" ] || { echo "[audit-debian-deb][error] missing file: $DEB_PATH" >&2; exit 1; }

echo "[audit-debian-deb] package: $DEB_PATH"
dpkg-deb -I "$DEB_PATH" | sed -n '1,80p'
echo "[audit-debian-deb] listing (dpkg-deb -c, first 200 lines):"
dpkg-deb -c "$DEB_PATH" | sed -n '1,200p'

tmpdir="$(mktemp -d)"
trap 'rm -rf "$tmpdir"' EXIT

dpkg-deb -x "$DEB_PATH" "$tmpdir"

echo "[audit-debian-deb] top directories (installed size):"
du -h -d 4 "$tmpdir" | sort -h | tail -n 20

echo "[audit-debian-deb] top files:"
find "$tmpdir" -type f -printf '%s %p\n' | sort -nr | head -n 30 | awk '{ sz=$1; $1=""; printf "%.2f MB %s\n", sz/1024/1024, substr($0,2)}'

echo "[audit-debian-deb] suspicious heavy patterns:"
find "$tmpdir" -type d \( -name '.vscode-test' -o -name 'node_modules' -o -name '.git' -o -name '__pycache__' -o -name 'tests' -o -name 'test' \) -print || true
