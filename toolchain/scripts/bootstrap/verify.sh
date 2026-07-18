#!/usr/bin/env sh
set -eu

ROOT_DIR="$(cd "$(dirname "$0")/../../.." && pwd)"
SEED_BIN="$ROOT_DIR/bin/vittec0"
FIXTURE="$ROOT_DIR/tests/bootstrap_native/main_proc.vit"

log() {
    printf "[verify] %s\n" "$1"
}

die() {
    printf "[verify][error] %s\n" "$1" >&2
    exit 1
}

[ -x "$SEED_BIN" ] || die "missing seed compiler: bin/vittec0"
[ -f "$FIXTURE" ] || die "missing bootstrap fixture"

TMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/vitte-bootstrap-verify.XXXXXX")"
trap 'rm -rf "$TMP_DIR"' EXIT HUP INT TERM

log "checking seed identity and trust-root contract"
"$SEED_BIN" --version
python3 "$ROOT_DIR/tools/check_bootstrap_seed_root.py" --artifacts

log "checking deterministic native IR"
"$SEED_BIN" dump-native-ir --src "$FIXTURE" > "$TMP_DIR/first.ir"
"$SEED_BIN" dump-native-ir --src "$FIXTURE" > "$TMP_DIR/second.ir"
cmp "$TMP_DIR/first.ir" "$TMP_DIR/second.ir" || die "native IR changed between identical runs"

log "checking deterministic native executable"
"$SEED_BIN" build-native --src "$FIXTURE" --out "$TMP_DIR/first.bin"
"$SEED_BIN" build-native --src "$FIXTURE" --out "$TMP_DIR/second.bin"
cmp "$TMP_DIR/first.bin" "$TMP_DIR/second.bin" || die "native executable changed between identical runs"
"$TMP_DIR/first.bin" --version >/dev/null || die "generated executable is not runnable"

log "seed bootstrap verification successful"
