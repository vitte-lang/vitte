#!/usr/bin/env bash
# ============================================================
# vitte — unit test runner
# Location: toolchain/scripts/test/unit.sh
# ============================================================

set -euo pipefail

# ----------------------------
# Configuration
# ----------------------------
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"
TARGET_DIR="${TARGET_DIR:-$ROOT_DIR/target}"
VITTEC_BIN="${VITTEC_BIN:-$TARGET_DIR/bin/vittec}"

TEST_ROOT="${TEST_ROOT:-$ROOT_DIR/tests/unit}"
PROFILE="${PROFILE:-debug}"           # debug | release
MODE="${MODE:-run}"                    # check | build | run
FILTER="${FILTER:-}"                   # substring filter on test name
TIMEOUT_SEC="${TIMEOUT_SEC:-5}"
VERBOSE="${VERBOSE:-0}"

# Layout
OUT_DIR="$TARGET_DIR/test-unit"
BIN_DIR="$OUT_DIR/bin"
LOG_DIR="$OUT_DIR/logs"

# ----------------------------
# Helpers
# ----------------------------
log() { printf "[unit-test] %s\n" "$*"; }
die() { printf "[unit-test][error] %s\n" "$*" >&2; exit 1; }

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
[ -d "$TEST_ROOT" ] || die "unit tests dir not found: $TEST_ROOT"

mkdir -p "$BIN_DIR" "$LOG_DIR"

log "profile=$PROFILE mode=$MODE timeout=${TIMEOUT_SEC}s"
[ -n "$FILTER" ] && log "filter=$FILTER"

# ----------------------------
# Discover tests
# Convention:
# tests/unit/<name>.vit
# ----------------------------
mapfile -t TESTS < <(find "$TEST_ROOT" -type f -name "*.vit" | sort)
[ "${#TESTS[@]}" -gt 0 ] || die "no unit tests found"

# ----------------------------
# Test loop
# ----------------------------
for src in "${TESTS[@]}"; do
  name="$(basename "$src" .vit)"

  if [ -n "$FILTER" ] && [[ "$name" != *"$FILTER"* ]]; then
    continue
  fi

  log "test: $name"

  BIN="$BIN_DIR/$name"
  STDOUT_LOG="$LOG_DIR/$name.stdout"
  STDERR_LOG="$LOG_DIR/$name.stderr"

  # ---------------- check ----------------
  case "$MODE" in
    check)
      run "$VITTEC_BIN" check "$src"
      continue
      ;;
  esac

  # ---------------- build ----------------
  run "$VITTEC_BIN" build "$src" \
    --profile "$PROFILE" \
    --output "$BIN"
  [ -x "$BIN" ] || die "binary not produced: $BIN"

  # ---------------- run ----------------
  case "$MODE" in
    run)
      set +e
      timeout_run "$BIN" >"$STDOUT_LOG" 2>"$STDERR_LOG"
      RC=$?
      set -e

      if [ "$RC" -ne 0 ]; then
        die "unit test failed: $name (rc=$RC)"
      fi
      ;;
  esac
done

log "unit tests complete"