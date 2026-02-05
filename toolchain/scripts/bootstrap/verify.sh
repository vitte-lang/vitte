#!/usr/bin/env sh
# ============================================================
# verify.sh — Vitte Toolchain
# Bootstrap verification (stage0 → stage2)
# ============================================================

set -eu

# ------------------------------------------------------------
# Helpers
# ------------------------------------------------------------

log() {
    printf "[verify] %s\n" "$1"
}

die() {
    printf "[verify][error] %s\n" "$1" >&2
    exit 1
}

checksum() {
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1" | awk '{print $1}'
    elif command -v shasum >/dev/null 2>&1; then
        shasum -a 256 "$1" | awk '{print $1}'
    else
        die "no sha256 tool available"
    fi
}

# ------------------------------------------------------------
# Paths
# ------------------------------------------------------------

ROOT_DIR="$(cd "$(dirname "$0")/../../.." && pwd)"
BIN_DIR="$ROOT_DIR/bin"

STAGE0_BIN="$BIN_DIR/vittec0"
STAGE1_BIN="$BIN_DIR/vittec1"
STAGE2_BIN="$BIN_DIR/vittec"

# ------------------------------------------------------------
# Checks
# ------------------------------------------------------------

log "root = $ROOT_DIR"

[ -x "$STAGE0_BIN" ] || die "missing vittec0 (stage0)"
[ -x "$STAGE1_BIN" ] || die "missing vittec1 (stage1)"
[ -x "$STAGE2_BIN" ] || die "missing vittec (stage2)"

# ------------------------------------------------------------
# Version checks
# ------------------------------------------------------------

log "checking versions"

"$STAGE0_BIN" --version || die "vittec0 version check failed"
"$STAGE1_BIN" --version || die "vittec1 version check failed"
"$STAGE2_BIN" --version || die "vittec version check failed"

# ------------------------------------------------------------
# Cross-compilation sanity
# ------------------------------------------------------------

log "cross-checking compiler outputs"

TMP_DIR="$ROOT_DIR/.bootstrap_verify"
SRC_DIR="$TMP_DIR/src"
OUT0="$TMP_DIR/out0"
OUT1="$TMP_DIR/out1"

rm -rf "$TMP_DIR"
mkdir -p "$SRC_DIR" "$OUT0" "$OUT1"

cat >"$SRC_DIR/hello.vit" <<EOF
proc main() {
    emit "hello bootstrap"
}
EOF

log "dump AST with vittec1"

"$STAGE1_BIN" parse --dump-ast "$SRC_DIR/hello.vit" > "$OUT0/ast.log"

log "dump AST with vittec (final)"

"$STAGE2_BIN" parse --dump-ast "$SRC_DIR/hello.vit" > "$OUT1/ast.log"

# ------------------------------------------------------------
# Hash comparison
# ------------------------------------------------------------

BIN0="$OUT0/ast.log"
BIN1="$OUT1/ast.log"

[ -f "$BIN0" ] || die "vittec1 output missing"
[ -f "$BIN1" ] || die "vittec output missing"

HASH0="$(checksum "$BIN0")"
HASH1="$(checksum "$BIN1")"

log "hash vittec1 = $HASH0"
log "hash vittec  = $HASH1"

if [ "$HASH0" != "$HASH1" ]; then
    die "non-deterministic output between vittec1 and vittec"
fi

# ------------------------------------------------------------
# ABI sanity (optional)
# ------------------------------------------------------------

if command -v file >/dev/null 2>&1; then
log "file format:"
    file "$BIN1"
fi

# ------------------------------------------------------------
# Cleanup
# ------------------------------------------------------------

rm -rf "$TMP_DIR"

log "bootstrap verification successful"
