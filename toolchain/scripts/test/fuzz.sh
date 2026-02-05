#!/usr/bin/env bash
# ============================================================
# vitte — compiler fuzz runner
# Location: toolchain/scripts/test/fuzz.sh
# ============================================================

set -euo pipefail

# ----------------------------
# Configuration
# ----------------------------
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"
TARGET_DIR="${TARGET_DIR:-$ROOT_DIR/target}"
VITTEC_BIN="${VITTEC_BIN:-$TARGET_DIR/bin/vittec}"

# Fuzz settings
MODE="${MODE:-smoke}"                 # smoke | ci | long
TIMEOUT_SEC="${TIMEOUT_SEC:-5}"        # per input
ITERATIONS="${ITERATIONS:-0}"          # 0 = auto by MODE
SEED="${SEED:-}"                       # optional deterministic seed
VERBOSE="${VERBOSE:-0}"                # 0 | 1

# Layout
FUZZ_ROOT="${FUZZ_ROOT:-$ROOT_DIR/tests/fuzz}"
CORPUS_DIR="$FUZZ_ROOT/corpus"
CRASH_DIR="$TARGET_DIR/fuzz/crashes"
ARTIFACT_DIR="$TARGET_DIR/fuzz/artifacts"
TMP_DIR="$TARGET_DIR/fuzz/tmp"

# ----------------------------
# Helpers
# ----------------------------
log() { printf "[fuzz] %s\n" "$*"; }
die() { printf "[fuzz][error] %s\n" "$*" >&2; exit 1; }

has() { command -v "$1" >/dev/null 2>&1; }

run() {
  if [ "$VERBOSE" = "1" ]; then log "exec: $*"; fi
  "$@"
}

timeout_run() {
  if has timeout; then
    timeout "$TIMEOUT_SEC" "$@"
  elif has gtimeout; then
    gtimeout "$TIMEOUT_SEC" "$@"
  else
    "$@"
  fi
}

# ----------------------------
# Init
# ----------------------------
cd "$ROOT_DIR"

[ -x "$VITTEC_BIN" ] || die "vittec not found or not executable: $VITTEC_BIN"

mkdir -p "$CORPUS_DIR" "$CRASH_DIR" "$ARTIFACT_DIR" "$TMP_DIR"

case "$MODE" in
  smoke)
    ITERATIONS="${ITERATIONS:-50}"
    ;;
  ci)
    ITERATIONS="${ITERATIONS:-300}"
    ;;
  long)
    ITERATIONS="${ITERATIONS:-0}"   # infinite
    ;;
  *)
    die "unknown MODE: $MODE"
    ;;
esac

log "mode=$MODE iterations=$ITERATIONS timeout=${TIMEOUT_SEC}s"

# ----------------------------
# Seed corpus (minimal)
# ----------------------------
if [ ! "$(ls -A "$CORPUS_DIR" 2>/dev/null)" ]; then
  log "seeding initial corpus"
  cat > "$CORPUS_DIR/empty.vit" <<'EOF'
program main
  ret 0
.end
EOF
fi

# ----------------------------
# Input generator
# ----------------------------
gen_input() {
  local out="$1"
  if [ -n "$SEED" ]; then
    awk "BEGIN{srand($SEED + NR)} {print}" >/dev/null
  fi

  # Very simple grammar-ish fuzz (expand later)
  cat <<'EOF' > "$out"
program fuzz
  make x = 1
  if x > 0
    ret x
  .end
.end
EOF
}

# ----------------------------
# Fuzz loop
# ----------------------------
i=0
while :; do
  i=$((i + 1))
  [ "$ITERATIONS" -ne 0 ] && [ "$i" -gt "$ITERATIONS" ] && break

  INPUT="$TMP_DIR/input_$i.vit"
  gen_input "$INPUT"

  log "iter=$i input=$INPUT"

  set +e
  timeout_run "$VITTEC_BIN" check "$INPUT" >"$ARTIFACT_DIR/out_$i.log" 2>&1
  RC=$?
  set -e

  if [ "$RC" -ne 0 ] && [ "$RC" -ne 124 ]; then
    CRASH="$CRASH_DIR/crash_$i.vit"
    cp "$INPUT" "$CRASH"
    log "crash detected (rc=$RC): $CRASH"
    exit 1
  fi
done

log "fuzzing complete (no crashes)"