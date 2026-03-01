#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
VITTE_BIN="${VITTE_BIN:-$ROOT_DIR/bin/vitte}"
SNAP_DIR="${SNAP_DIR:-$ROOT_DIR/target/snapshots/crash-report}"

log() { printf "[crash-report-snapshots] %s\n" "$*"; }
die() { printf "[crash-report-snapshots][error] %s\n" "$*" >&2; exit 1; }

[ -x "$VITTE_BIN" ] || die "missing binary: $VITTE_BIN (run make build)"
mkdir -p "$SNAP_DIR"

log "backend failure snapshot"
set +e
"$VITTE_BIN" build --target arduino-uno --emit-obj "$ROOT_DIR/tests/strict_ok.vit" >"$SNAP_DIR/backend.fail.out" 2>&1
rc=$?
set -e
[ "$rc" -ne 0 ] || die "expected backend failure"
grep -Fq "[vittec][crash-report]" "$SNAP_DIR/backend.fail.out" || die "missing crash report header"
grep -Fq "reason: backend/linker failure" "$SNAP_DIR/backend.fail.out" || die "missing reason field"
grep -Fq "stage: backend" "$SNAP_DIR/backend.fail.out" || die "missing stage field"
grep -Fq "exit-code:" "$SNAP_DIR/backend.fail.out" || die "missing exit-code field"
grep -Fq "repro: vitte reduce --stage backend" "$SNAP_DIR/backend.fail.out" || die "missing repro field"

latest_meta_txt="$(find "$ROOT_DIR/.vitte-crash" -type f -name metadata.txt | sort | tail -n1)"
latest_meta_json="$(find "$ROOT_DIR/.vitte-crash" -type f -name metadata.json | sort | tail -n1)"
[ -n "$latest_meta_txt" ] || die "metadata.txt not generated"
[ -n "$latest_meta_json" ] || die "metadata.json not generated"
cp "$latest_meta_txt" "$SNAP_DIR/metadata.txt.snapshot"
cp "$latest_meta_json" "$SNAP_DIR/metadata.json.snapshot"
grep -Fq "reason=pipeline failed" "$SNAP_DIR/metadata.txt.snapshot" || die "metadata.txt missing reason"
grep -Fq "stage=backend" "$SNAP_DIR/metadata.txt.snapshot" || die "metadata.txt missing stage"
grep -Fq "\"reason\": \"pipeline failed\"" "$SNAP_DIR/metadata.json.snapshot" || die "metadata.json missing reason"
grep -Fq "\"stage\": \"backend\"" "$SNAP_DIR/metadata.json.snapshot" || die "metadata.json missing stage"

log "signal trap snapshot"
set +e
VITTE_TEST_TRAP=SIGABRT "$VITTE_BIN" --help >"$SNAP_DIR/signal.fail.out" 2>&1
signal_rc=$?
set -e
[ "$signal_rc" -eq 134 ] || die "expected signal trap rc=134, got rc=$signal_rc"
grep -Fq "[vittec][crash-report]" "$SNAP_DIR/signal.fail.out" || die "missing signal crash report"
grep -Fq "reason: fatal signal" "$SNAP_DIR/signal.fail.out" || die "missing signal reason"
grep -Fq "signal: SIGABRT (6)" "$SNAP_DIR/signal.fail.out" || die "missing signal detail"

log "OK"
