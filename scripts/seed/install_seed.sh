#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
SEED="$ROOT_DIR/toolchain/seed/vittec0.seed"
OUT_DIR="$ROOT_DIR/bin"
OUT="$OUT_DIR/vittec0"

if [ ! -f "$SEED" ]; then
    printf '[bootstrap-seed][error] missing seed artifact: %s\n' "$SEED" >&2
    exit 1
fi

mkdir -p "$OUT_DIR"
cp "$SEED" "$OUT"
chmod 755 "$OUT"

cd "$ROOT_DIR"
python3 tools/check_bootstrap_seed_root.py --artifacts

printf '[bootstrap-seed] installed %s\n' "$OUT"
