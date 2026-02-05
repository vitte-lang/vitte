#!/usr/bin/env bash
# ============================================================
# vitte — compiler test runner
# Location: toolchain/scripts/test/compiler.sh
# ============================================================

set -euo pipefail

# ----------------------------
# Configuration
# ----------------------------
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"
TARGET_DIR="${TARGET_DIR:-$ROOT_DIR/target}"
VITTEC_BIN="${VITTEC_BIN:-$TARGET_DIR/bin/vittec}"

TEST_ROOT="${TEST_ROOT:-$ROOT_DIR/tests/compiler}"
MODE="${MODE:-all}"                   # all | parse | check | codegen | link
PROFILE="${PROFILE:-debug}"           # debug | release
UPDATE_GOLDEN="${UPDATE_GOLDEN:-0}"   # 1 to overwrite expected outputs
VERBOSE="${VERBOSE:-0}"               # 0 | 1

# Layout conventions
SRC_EXT=".vit"
OUT_DIR="$TARGET_DIR/test-compiler"
TMP_DIR="$OUT_DIR/tmp"

# ----------------------------
# Helpers
# ----------------------------
log() { printf "[compiler-test] %s\n" "$*"; }
die() { printf "[compiler-test][error] %s\n" "$*" >&2; exit 1; }

run() {
  if [ "$VERBOSE" = "1" ]; then log "exec: $*"; fi
  "$@"
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
[ -d "$TEST_ROOT" ] || die "test directory not found: $TEST_ROOT"

mkdir -p "$TMP_DIR"

log "mode=$MODE profile=$PROFILE"
log "tests=$TEST_ROOT"

# ----------------------------
# Discover tests
# ----------------------------
mapfile -t TESTS < <(find "$TEST_ROOT" -type f -name "*$SRC_EXT" | sort)
[ "${#TESTS[@]}" -gt 0 ] || die "no compiler tests found"

# ----------------------------
# Test loop
# ----------------------------
FAIL=0

for src in "${TESTS[@]}"; do
  name="$(basename "$src" "$SRC_EXT")"
  dir="$(dirname "$src")"

  log "test: $name"

  case "$MODE" in
    all|parse)
      GOT="$TMP_DIR/$name.parse"
      run "$VITTEC_BIN" parse "$src" > "$GOT"
      if [ -f "$dir/$name.parse.expect" ]; then
        expect_file "$GOT" "$dir/$name.parse.expect"
      fi
      ;;
  esac

  case "$MODE" in
    all|check)
      GOT="$TMP_DIR/$name.check"
      run "$VITTEC_BIN" check "$src" > "$GOT"
      if [ -f "$dir/$name.check.expect" ]; then
        expect_file "$GOT" "$dir/$name.check.expect"
      fi
      ;;
  esac

  case "$MODE" in
    all|codegen)
      GOT="$TMP_DIR/$name.codegen"
      run "$VITTEC_BIN" emit "$src" --profile "$PROFILE" > "$GOT"
      if [ -f "$dir/$name.codegen.expect" ]; then
        expect_file "$GOT" "$dir/$name.codegen.expect"
      fi
      ;;
  esac

  case "$MODE" in
    all|link)
      BIN="$TMP_DIR/$name.bin"
      run "$VITTEC_BIN" build "$src" \
        --profile "$PROFILE" \
        --output "$BIN"
      [ -x "$BIN" ] || die "binary not produced: $BIN"
      ;;
  esac
done

log "compiler tests complete"