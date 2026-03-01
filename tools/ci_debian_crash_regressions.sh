#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
VITTE_BIN="${VITTE_BIN:-$ROOT_DIR/bin/vitte}"
OUT_DIR="${OUT_DIR:-$ROOT_DIR/target/ci-crash}"

log() { printf "[ci-debian-crash] %s\n" "$*"; }
die() { printf "[ci-debian-crash][error] %s\n" "$*" >&2; exit 1; }

expect_ok() {
  local name="$1"
  shift
  log "$name"
  "$@" >/tmp/ci-debian-crash.out 2>&1 || {
    cat /tmp/ci-debian-crash.out
    die "expected success: $name"
  }
}

expect_fail() {
  local name="$1"
  shift
  log "$name"
  set +e
  "$@" >/tmp/ci-debian-crash.out 2>&1
  local rc=$?
  set -e
  if [ "$rc" -eq 0 ]; then
    cat /tmp/ci-debian-crash.out
    die "expected failure: $name"
  fi
}

[ -x "$VITTE_BIN" ] || die "missing binary: $VITTE_BIN (run make build)"
mkdir -p "$OUT_DIR"

expect_ok "doctor" "$VITTE_BIN" doctor
expect_ok "parse corpus" "$VITTE_BIN" parse --stage parse "$ROOT_DIR/tests/strict_ok.vit"
expect_ok "check corpus" "$VITTE_BIN" check "$ROOT_DIR/tests/strict_ok.vit"
expect_ok "build corpus" "$VITTE_BIN" build "$ROOT_DIR/tests/strict_ok.vit" -o "$OUT_DIR/strict_ok.out"

expect_fail "check negative exit code" "$VITTE_BIN" check "$ROOT_DIR/tests/proc_expr.vit"

rm -rf "$ROOT_DIR/.vitte-crash"
expect_fail "backend failure emits crash repro" \
  "$VITTE_BIN" build --target arduino-uno --emit-obj "$ROOT_DIR/tests/strict_ok.vit"
grep -Fq "[crash] repro hint:" /tmp/ci-debian-crash.out || {
  cat /tmp/ci-debian-crash.out
  die "missing crash repro hint on backend failure"
}
find "$ROOT_DIR/.vitte-crash" -type f -name metadata.txt | grep -q . || die "missing crash metadata artifact"
find "$ROOT_DIR/.vitte-crash" -type f -name metadata.json | grep -q . || die "missing crash metadata.json artifact"

tmp_bin_dir="$(mktemp -d)"
ln -sf "$VITTE_BIN" "$tmp_bin_dir/vittec"
ln -sf "$VITTE_BIN" "$tmp_bin_dir/vitte-linker"
expect_ok "compat wrapper banner vittec" "$tmp_bin_dir/vittec" --help
grep -Fq "[compat] vittec mode via vitte" /tmp/ci-debian-crash.out || die "missing vittec compat banner"
expect_ok "compat wrapper banner vitte-linker" "$tmp_bin_dir/vitte-linker" --help
grep -Fq "[compat] vitte-linker mode via vitte" /tmp/ci-debian-crash.out || die "missing vitte-linker compat banner"

set +e
VITTE_TEST_TRAP=SIGABRT "$VITTE_BIN" --help >/tmp/ci-debian-crash.signal 2>&1
signal_rc=$?
set -e
[ "$signal_rc" -eq 134 ] || die "signal crash-report smoke test failed: expected rc=134 got rc=$signal_rc"
grep -Fq "[vittec][crash-report]" /tmp/ci-debian-crash.signal || die "missing crash report in signal trap test"

log "OK"
