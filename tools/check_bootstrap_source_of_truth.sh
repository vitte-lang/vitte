#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
CONFIG="$ROOT_DIR/toolchain/bootstrap-config.json"
MANIFEST="$ROOT_DIR/toolchain/seed/manifest.txt"
SEED="$ROOT_DIR/toolchain/seed/vittec0.seed"
SEED_SOURCE="$ROOT_DIR/toolchain/seed/src/main.vit"

die() {
  echo "[seed-source-of-truth][error] $1" >&2
  exit 1
}

[ -f "$CONFIG" ] || die "missing toolchain/bootstrap-config.json"
[ -f "$MANIFEST" ] || die "missing toolchain/seed/manifest.txt"
[ -f "$SEED" ] || die "missing toolchain/seed/vittec0.seed"
[ -f "$SEED_SOURCE" ] || die "missing toolchain/seed/src/main.vit"

python3 "$ROOT_DIR/tools/check_bootstrap_stage_chain.py" >/dev/null || die "seed chain contract failed"

grep -F '"bootstrap_stages": 1' "$CONFIG" >/dev/null || die "bootstrap config must declare one seed stage"
grep -F '"name": "seed"' "$CONFIG" >/dev/null || die "bootstrap config must name the trust root seed"
grep -F '"compiler": "toolchain/seed/vittec0.seed"' "$CONFIG" >/dev/null || die "bootstrap compiler must be vittec0.seed"
grep -F '"artifact": "toolchain/seed/vittec0.seed"' "$CONFIG" >/dev/null || die "bootstrap artifact must be vittec0.seed"
grep -F '"output": "bin/vittec0"' "$CONFIG" >/dev/null || die "bootstrap output must be bin/vittec0"

grep -F 'source_file=toolchain/seed/src/main.vit' "$MANIFEST" >/dev/null || die "seed manifest source_file drifted"
grep -F 'seed_file=toolchain/seed/vittec0.seed' "$MANIFEST" >/dev/null || die "seed manifest seed_file drifted"

echo "[seed-source-of-truth] ok"
