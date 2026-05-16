#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

SRC="toolchain/src/bootstrap_vitte/bin/main.vit"
OUT="target/bootstrap/bootstrap_vitte_gate"

[ -x "bin/vitte" ] || { echo "[bootstrap-vitte][error] missing bin/vitte" >&2; exit 2; }
[ -f "$SRC" ] || { echo "[bootstrap-vitte][error] missing $SRC" >&2; exit 2; }

mkdir -p target/bootstrap target/reports/bootstrap

echo "[bootstrap-vitte] validating bootstrap_vitte sources"
bin/vitte check "$SRC"

echo "[bootstrap-vitte] compiling native bootstrap gate"
bin/vitte build-native --src "$SRC" --out "$OUT"
[ -x "$OUT" ] || { echo "[bootstrap-vitte][error] failed to produce native gate artifact" >&2; exit 3; }

echo "[bootstrap-vitte] executing native hard gate invariants"
sh "$OUT"

step_start() { printf '[bootstrap-vitte][step] %s\n' "$1"; }
t0="$(date +%s)"

step_start "seed-verify"
make --no-print-directory seed-verify

step_start "bootstrap-all"
make --no-print-directory bootstrap-all-legacy

step_start "bootstrap-parity"
make --no-print-directory bootstrap-parity

step_start "bootstrap-native-snapshots"
make --no-print-directory bootstrap-native-snapshots

t1="$(date +%s)"
dur="$((t1 - t0))"
cat > target/reports/bootstrap/hard_gate_native.json <<EOF
{
  "schema": "bootstrap.vitte.v1",
  "bootstrap": "bootstrap_vitte",
  "strict": true,
  "status": "ok",
  "duration_sec": $dur,
  "steps": [
    "seed-verify",
    "bootstrap-all",
    "bootstrap-parity",
    "bootstrap-native-snapshots"
  ]
}
EOF

echo "[bootstrap-vitte] hard gate completed"
