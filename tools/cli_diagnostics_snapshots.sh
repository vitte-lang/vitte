#!/usr/bin/env sh
set -eu

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT_DIR/bin/vitte"
SNAP_DIR="$ROOT_DIR/tests/bootstrap_native"
TMP_DIR="$(mktemp -d "$ROOT_DIR/target/cli-diag-snapshots.XXXXXX")"
trap 'rm -rf "$TMP_DIR"' EXIT HUP INT TERM

die() {
    printf '[cli-diagnostics-snapshots][error] %s\n' "$1" >&2
    exit 1
}

norm_file() {
    in_file="$1"
    out_file="$2"
    sed "s|$ROOT_DIR/|/ROOT/|g" "$in_file" > "$out_file"
}

FIXTURE="tests/bootstrap_native/bad_unknown_const.vit"

if "$BIN" --strict parse "$FIXTURE" >"$TMP_DIR/strict.out" 2>"$TMP_DIR/strict.err"; then
    die "strict diagnostic fixture unexpectedly succeeded"
fi
norm_file "$TMP_DIR/strict.err" "$TMP_DIR/strict.norm.err"
diff -u "$SNAP_DIR/strict_diag.parse.bad_unknown_const.err.must" "$TMP_DIR/strict.norm.err" || die "strict text diagnostics snapshot drift"

printf '[cli-diagnostics-snapshots] ok\n'
