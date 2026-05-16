#!/usr/bin/env sh
set -eu
ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BIN_DIR="$ROOT_DIR/bin"
MANIFEST="$ROOT_DIR/toolchain/seed/manifest.txt"

"$ROOT_DIR/scripts/seed/verify_seed.sh"
seed_file="$(awk -F= '/^seed_file=/{print $2}' "$MANIFEST")"
seed_path="$ROOT_DIR/$seed_file"

mkdir -p "$BIN_DIR"
tmp_seed="$BIN_DIR/.vittec0.$$"
cp "$seed_path" "$tmp_seed"
mv "$tmp_seed" "$BIN_DIR/vittec0"
chmod +x "$BIN_DIR/vittec0"
echo "[seed-install] installed $BIN_DIR/vittec0"
