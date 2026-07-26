#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
CONFIG="$ROOT_DIR/toolchain/vitte-bootstrap.toml"

CONFIG_JSON=$(cd "$ROOT_DIR" && python3 tools/bootstrap_config.py --json "$CONFIG")
SEED_REL=$(printf '%s\n' "$CONFIG_JSON" | python3 -c 'import json,sys; print(json.load(sys.stdin)["seed"]["artifact"])')
OUT_REL=$(printf '%s\n' "$CONFIG_JSON" | python3 -c 'import json,sys; print(json.load(sys.stdin)["compiler"]["stage0"])')
SEED="$ROOT_DIR/$SEED_REL"
OUT="$ROOT_DIR/$OUT_REL"
OUT_DIR=${OUT%/*}

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
