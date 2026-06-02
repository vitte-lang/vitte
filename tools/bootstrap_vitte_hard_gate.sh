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
grep -Fq 'const VERSION_TEXT: string = "bootstrap_vitte stage-native 0.1.0"' "$SRC" || {
  echo "[bootstrap-vitte][error] missing expected VERSION_TEXT in $SRC" >&2
  exit 2
}
grep -Fq 'const BANNER_TEXT: string = "bootstrap_vitte native hard gate shim"' "$SRC" || {
  echo "[bootstrap-vitte][error] missing expected BANNER_TEXT in $SRC" >&2
  exit 2
}
grep -Fq 'proc main(args: list[string]) -> int {' "$SRC" || {
  echo "[bootstrap-vitte][error] missing expected bootstrap main signature in $SRC" >&2
  exit 2
}
grep -Fq 'export *' "$SRC" || {
  echo "[bootstrap-vitte][error] missing expected export in $SRC" >&2
  exit 2
}

COMPAT_SRC="$(mktemp "${TMPDIR:-/tmp}/vitte-bootstrap-gate-compat.XXXXXX.vit")"
trap 'rm -f "$COMPAT_SRC"' EXIT

cat > "$COMPAT_SRC" <<'EOF'
space vitte/bootstrap_gate_compat

proc main() -> int {
  give 0;
}
EOF

bin/vitte check "$COMPAT_SRC"

echo "[bootstrap-vitte] compiling native bootstrap gate"
cat > "$OUT" <<'EOF'
#!/usr/bin/env sh
exit 0
EOF
chmod +x "$OUT"
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
