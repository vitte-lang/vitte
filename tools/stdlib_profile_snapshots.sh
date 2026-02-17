#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
TEST_DIR="${TEST_DIR:-$ROOT_DIR/tests/stdlib_profiles}"

log() { printf "[stdlib-profile] %s\n" "$*"; }
die() { printf "[stdlib-profile][error] %s\n" "$*" >&2; exit 1; }

[ -x "$BIN" ] || die "missing binary: $BIN"
[ -d "$TEST_DIR" ] || die "missing tests dir: $TEST_DIR"

run_expect_ok() {
  local profile="$1"
  local src="$2"
  set +e
  local out
  out="$("$BIN" check --lang=en --stdlib-profile "$profile" "$src" 2>&1)"
  local rc=$?
  set -e
  if [ "$rc" -ne 0 ]; then
    printf "%s\n" "$out"
    die "expected success: profile=$profile src=$src"
  fi
}

run_expect_err() {
  local profile="$1"
  local src="$2"
  local needle="$3"
  set +e
  local out
  out="$("$BIN" check --lang=en --stdlib-profile "$profile" "$src" 2>&1)"
  local rc=$?
  set -e
  if [ "$rc" -eq 0 ]; then
    printf "%s\n" "$out"
    die "expected failure: profile=$profile src=$src"
  fi
  if ! grep -Fq "$needle" <<<"$out"; then
    printf "%s\n" "$out"
    die "expected message '$needle' for profile=$profile src=$src"
  fi
}

core="$TEST_DIR/use_core.vit"
net="$TEST_DIR/use_net.vit"
kernel="$TEST_DIR/use_kernel_console.vit"
arduino="$TEST_DIR/use_arduino_serial.vit"

log "minimal profile"
run_expect_ok minimal "$core"
run_expect_err minimal "$net" "error[E1010]"
run_expect_err minimal "$kernel" "error[E1010]"
run_expect_err minimal "$arduino" "error[E1010]"

log "kernel profile"
run_expect_ok kernel "$core"
run_expect_ok kernel "$kernel"
run_expect_err kernel "$net" "error[E1010]"
run_expect_err kernel "$arduino" "error[E1010]"

log "arduino profile"
run_expect_ok arduino "$core"
run_expect_ok arduino "$arduino"
run_expect_err arduino "$net" "error[E1010]"
run_expect_err arduino "$kernel" "error[E1010]"

log "full profile"
run_expect_ok full "$core"
run_expect_ok full "$net"
run_expect_ok full "$kernel"
run_expect_ok full "$arduino"

log "OK"
