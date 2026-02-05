#!/usr/bin/env bash
# ============================================================
# vitte — integration test runner
# Location: toolchain/scripts/test/integration.sh
# ============================================================

set -euo pipefail

# ----------------------------
# Configuration
# ----------------------------
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"
TARGET_DIR="${TARGET_DIR:-$ROOT_DIR/target}"
VITTEC_BIN="${VITTEC_BIN:-$TARGET_DIR/bin/vittec}"

TEST_ROOT="${TEST_ROOT:-$ROOT_DIR/tests/integration}"
PROFILE="${PROFILE:-debug}"           # debug | release
MODE="${MODE:-all}"                    # all | build | run
TIMEOUT_SEC="${TIMEOUT_SEC:-10}"       # per test
UPDATE_GOLDEN="${UPDATE_GOLDEN:-0}"    # 1 to overwrite expected outputs
VERBOSE="${VERBOSE:-0}"                # 0 | 1

# Layout
OUT_DIR="$TARGET_DIR/test-integration"
BIN_DIR="$OUT_DIR/bin"
LOG_DIR="$OUT_DIR/logs"
TMP_DIR="$OUT_DIR/tmp"

# ----------------------------
# Helpers
# ----------------------------
log() { printf "[integration] %s\n" "$*"; }
die() { printf "[integration][error] %s\n" "$*" >&2; exit 1; }

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

expect_file() {
  local got="$1" exp="$2"
  if [ "$UPDATE_GOLDEN" = "1" ]; then
    mkdir -p "$(dirname "$exp")"
    cp "$got" "$exp"
    log "updated golden: $exp"
  else
    diff -u "$exp" "$got"
  fi
}

# ----------------------------
# Init
# ----------------------------
cd "$ROOT_DIR"

[ -x "$VITTEC_BIN" ] || die "vittec not found or not executable: $VITTEC_BIN"
[ -d "$TEST_ROOT" ] || die "integration tests dir not found: $TEST_ROOT"

mkdir -p "$BIN_DIR" "$LOG_DIR" "$TMP_DIR"

log "profile=$PROFILE mode=$MODE timeout=${TIMEOUT_SEC}s"
log "tests=$TEST_ROOT"

# ----------------------------
# Discover tests
# Convention:
# tests/integration/<name>/
#   ├─ main.vit
#   ├─ args.txt            (optional)
#   ├─ env.sh              (optional)
#   ├─ stdout.expect       (optional)
#   └─ stderr.expect       (optional)
# ----------------------------
mapfile -t TESTS < <(find "$TEST_ROOT" -mindepth 1 -maxdepth 1 -type d | sort)
[ "${#TESTS[@]}" -gt 0 ] || die "no integration tests found"

# ----------------------------
# Test loop
# ----------------------------
for tdir in "${TESTS[@]}"; do
  name="$(basename "$tdir")"
  src="$tdir/main.vit"
  [ -f "$src" ] || die "missing main.vit in $tdir"

  log "test: $name"

  BIN="$BIN_DIR/$name"
  STDOUT_LOG="$LOG_DIR/$name.stdout"
  STDERR_LOG="$LOG_DIR/$name.stderr"

  # Optional env
  if [ -f "$tdir/env.sh" ]; then
    log "loading env for $name"
    # shellcheck source=/dev/null
    source "$tdir/env.sh"
  fi

  # ---------------- build ----------------
  case "$MODE" in
    all|build)
      run "$VITTEC_BIN" build "$src" \
        --profile "$PROFILE" \
        --output "$BIN"
      [ -x "$BIN" ] || die "binary not produced: $BIN"
      ;;
  esac

  # ---------------- run ----------------
  case "$MODE" in
    all|run)
      ARGS=()
      if [ -f "$tdir/args.txt" ]; then
        # shellcheck disable=SC2206
        ARGS=( $(cat "$tdir/args.txt") )
      fi

      set +e
      timeout_run "$BIN" "${ARGS[@]}" >"$STDOUT_LOG" 2>"$STDERR_LOG"
      RC=$?
      set -e

      # Expected outputs
      if [ -f "$tdir/stdout.expect" ]; then
        expect_file "$STDOUT_LOG" "$tdir/stdout.expect"
      fi
      if [ -f "$tdir/stderr.expect" ]; then
        expect_file "$STDERR_LOG" "$tdir/stderr.expect"
      fi

      # Optional expected exit code
      if [ -f "$tdir/exit.expect" ]; then
        EXP_RC="$(cat "$tdir/exit.expect")"
        [ "$RC" -eq "$EXP_RC" ] || die "exit code mismatch for $name (got=$RC exp=$EXP_RC)"
      fi
      ;;
  esac
done

log "integration tests complete"