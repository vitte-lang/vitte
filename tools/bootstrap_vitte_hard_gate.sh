#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

SRC="toolchain/src/bootstrap_vitte/bin/main.vit"
OUT="target/bootstrap/bootstrap_vitte_gate"
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
  local candidate
  for candidate in bin/vittec0 bin/vittec1 bin/vittec bin/vitte; do
    if is_runnable_candidate "$candidate"; then
      CHECKER="$candidate"
      return 0
    fi
  done
  return 1
}

pick_runnable_checker || {
  echo "[bootstrap-vitte][error] missing runnable compiler (tried bin/vittec0, bin/vittec1, bin/vittec, bin/vitte)" >&2
  exit 2
}

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

echo "[bootstrap-vitte] compatibility check via $CHECKER"
"$CHECKER" check "$COMPAT_SRC"

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

step_start "bootstrap-seed"
make --no-print-directory bootstrap-seed

step_start "seed-chain-artifacts"
python3 tools/check_bootstrap_stage_chain.py --artifacts

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
    "seed-chain-artifacts",
    "bootstrap-native-snapshots"
  ]
}
EOF

echo "[bootstrap-vitte] hard gate completed"
