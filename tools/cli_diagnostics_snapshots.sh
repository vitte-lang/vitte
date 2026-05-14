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

if "$BIN" --strict-diagnostics parse "$FIXTURE" >"$TMP_DIR/strict.out" 2>"$TMP_DIR/strict.err"; then
    die "strict diagnostic fixture unexpectedly succeeded"
fi
norm_file "$TMP_DIR/strict.err" "$TMP_DIR/strict.norm.err"
diff -u "$SNAP_DIR/strict_diag.parse.bad_unknown_const.err.must" "$TMP_DIR/strict.norm.err" || die "strict text diagnostics snapshot drift"

if "$BIN" --strict-diagnostics --diagnostics-format json parse "$FIXTURE" >"$TMP_DIR/strict_json.out" 2>"$TMP_DIR/strict_json.err"; then
    die "strict json diagnostic fixture unexpectedly succeeded"
fi
norm_file "$TMP_DIR/strict_json.err" "$TMP_DIR/strict_json.norm.err"
diff -u "$SNAP_DIR/strict_diag_json.parse.bad_unknown_const.err.must" "$TMP_DIR/strict_json.norm.err" || die "strict json diagnostics snapshot drift"

python3 - <<'PY' "$TMP_DIR/strict_json.norm.err"
import json, pathlib, sys
text = pathlib.Path(sys.argv[1]).read_text(encoding='utf-8')
obj = json.loads(text)
assert isinstance(obj, dict)
assert isinstance(obj.get("diagnostics"), list)
assert len(obj["diagnostics"]) >= 1
PY

printf '[cli-diagnostics-snapshots] ok\n'
