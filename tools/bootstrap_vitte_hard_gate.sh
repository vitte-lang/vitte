#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

SRC="src/vitte/compiler/main.vit"
OUT="target/bootstrap/bootstrap_vitte_gate"
EXECUTION_FIXTURE="tests/bootstrap_native/main_const_int.vit"
EXECUTION_OUT="target/bootstrap/bootstrap_vitte_execution_probe"
CHECKER=""

[ -f "$SRC" ] || { echo "[bootstrap-vitte][error] missing $SRC" >&2; exit 2; }

is_host_compatible_machine_binary() {
  local candidate kind host_os host_arch
  candidate="$1"
  kind="$(LC_ALL=C file -b "$candidate" 2>/dev/null || true)"
  host_os="$(uname -s)"
  host_arch="$(uname -m)"
  case "$kind" in
    *Mach-O*)
      [ "$host_os" = "Darwin" ] || return 1
      case "$host_arch:$kind" in
        arm64:*arm64*|x86_64:*x86_64*|x86_64:*universal*) return 0 ;;
      esac
      return 1
      ;;
    *ELF*)
      [ "$host_os" = "Linux" ] || return 1
      case "$host_arch:$kind" in
        x86_64:*x86-64*|aarch64:*ARM\ aarch64*|arm64:*ARM\ aarch64*) return 0 ;;
      esac
      return 1
      ;;
    *PE32*|*PE32+*)
      [ "${OS:-}" = "Windows_NT" ] || return 1
      return 0
      ;;
  esac
  return 1
}

is_runnable_candidate() {
  local candidate first_line
  candidate="$1"
  [ -x "$candidate" ] || return 1
  first_line="$(sed -n '1p' "$candidate" 2>/dev/null || true)"
  case "$first_line" in
    '#!'*)
      "$candidate" --help >/dev/null 2>&1
      return $?
      ;;
  esac
  is_host_compatible_machine_binary "$candidate" || return 1
  "$candidate" --help >/dev/null 2>&1
}

pick_runnable_checker() {
  if is_runnable_candidate "bin/vittec0"; then
    CHECKER="bin/vittec0"
    return 0
  fi
  return 1
}

pick_runnable_checker || {
  echo "[bootstrap-vitte][error] missing runnable seed compiler: bin/vittec0" >&2
  exit 2
}

mkdir -p target/bootstrap target/reports/bootstrap

echo "[bootstrap-vitte] validating canonical compiler source"
grep -Fq 'const VERSION_TEXT: string = "vittec vitte-compiler 0.1.0"' "$SRC" || {
  echo "[bootstrap-vitte][error] missing expected VERSION_TEXT in $SRC" >&2
  exit 2
}
grep -Fq 'const BANNER_TEXT: string = "vittec Vitte compiler driver"' "$SRC" || {
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

echo "[bootstrap-vitte] compatibility check via $CHECKER"
"$CHECKER" check "$EXECUTION_FIXTURE"

echo "[bootstrap-vitte] compiling bootstrap entry with seed"
rm -f "$OUT" "$EXECUTION_OUT"
"$CHECKER" build-native --src "$SRC" --out "$OUT"
tools/require_native_artifact.sh "$OUT" compiler

echo "[bootstrap-vitte] executing native hard gate invariants"
version_output=$("$OUT" --version)
case "$version_output" in
  vittec*) ;;
  *) echo "[bootstrap-vitte][error] generated compiler returned invalid version: $version_output" >&2; exit 3 ;;
esac
"$OUT" build-native --src "$EXECUTION_FIXTURE" --out "$EXECUTION_OUT"
tools/require_native_artifact.sh "$EXECUTION_OUT"
set +e
"$EXECUTION_OUT"
execution_rc=$?
set -e
[ "$execution_rc" -eq 9 ] || {
  echo "[bootstrap-vitte][error] generated code returned $execution_rc, expected 9" >&2
  exit 3
}

step_start() { printf '[bootstrap-vitte][step] %s\n' "$1"; }
t0="$(date +%s)"

step_start "seed-verify"
make --no-print-directory seed-verify

step_start "bootstrap-seed"
make --no-print-directory bootstrap-seed

step_start "seed-root-artifacts"
python3 tools/check_bootstrap_seed_root.py --artifacts

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
    "bootstrap-seed",
    "seed-root-artifacts",
    "bootstrap-native-snapshots"
  ]
}
EOF

echo "[bootstrap-vitte] hard gate completed"
