#!/usr/bin/env sh
# vitte/rust/scripts/gen_headers.sh
#
# MAX header generator for Vitte Rust FFI (cbindgen).
#
# Generates:
#   - rust/include-gen/vitte_rust_api.h     (from crate: vitte_rust_api)
#
# Optional (if you add more FFI crates later):
#   - add them to CRATES below.
#
# Usage:
#   ./rust/scripts/gen_headers.sh
#   ./rust/scripts/gen_headers.sh --check
#   ./rust/scripts/gen_headers.sh --clean
#   ./rust/scripts/gen_headers.sh --verbose
#
# Flags:
#   --check     : do not overwrite; fail if output would change
#   --clean     : delete include-gen before regenerating
#   --verbose   : print commands and paths
#   --no-config : ignore rust/cbindgen.toml (use cbindgen defaults)
#
# Requirements:
#   - cbindgen installed: cargo install cbindgen
#
# Notes:
#   - Determinism depends on cbindgen.toml (sorting, guards, etc.)
#   - Script avoids GNU-only features; works on macOS/Linux sh.

set -eu

# -----------------------------------------------------------------------------
# Args
# -----------------------------------------------------------------------------

CHECK=0
CLEAN=0
VERBOSE=0
NO_CONFIG=0

for a in "$@"; do
  case "$a" in
    --check) CHECK=1 ;;
    --clean) CLEAN=1 ;;
    --verbose) VERBOSE=1 ;;
    --no-config) NO_CONFIG=1 ;;
    -h|--help)
      cat <<'EOF'
gen_headers.sh — generate C headers for Vitte Rust FFI (cbindgen)

Usage:
  ./rust/scripts/gen_headers.sh [--check] [--clean] [--verbose] [--no-config]

Flags:
  --check     : do not overwrite; fail if output would change
  --clean     : delete rust/include-gen before regenerating
  --verbose   : print commands and paths
  --no-config : ignore rust/cbindgen.toml

EOF
      exit 0
      ;;
    *)
      echo "error: unknown arg: $a" >&2
      exit 2
      ;;
  esac
done

log() {
  if [ "$VERBOSE" -eq 1 ]; then
    echo "$@"
  fi
}

die() {
  echo "error: $*" >&2
  exit 1
}

need_cmd() {
  command -v "$1" >/dev/null 2>&1 || die "missing command: $1"
}

# -----------------------------------------------------------------------------
# Resolve paths
# -----------------------------------------------------------------------------

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
RUST_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
ROOT_DIR="$(CDPATH= cd -- "$RUST_DIR/.." && pwd)"

OUT_DIR="$RUST_DIR/include-gen"
TMP_DIR="$OUT_DIR/.tmp-gen-headers"

CBINDGEN_TOML="$RUST_DIR/cbindgen.toml"

log "SCRIPT_DIR=$SCRIPT_DIR"
log "RUST_DIR=$RUST_DIR"
log "ROOT_DIR=$ROOT_DIR"
log "OUT_DIR=$OUT_DIR"

# -----------------------------------------------------------------------------
# Tools
# -----------------------------------------------------------------------------

need_cmd cbindgen

# diff is used for --check comparisons; most systems have it.
if [ "$CHECK" -eq 1 ]; then
  need_cmd diff
fi

# -----------------------------------------------------------------------------
# Clean + mkdir
# -----------------------------------------------------------------------------

if [ "$CLEAN" -eq 1 ]; then
  log "clean: rm -rf $OUT_DIR"
  rm -rf "$OUT_DIR"
fi

mkdir -p "$OUT_DIR"
rm -rf "$TMP_DIR"
mkdir -p "$TMP_DIR"

# Decide config arg
CONFIG_ARG=""
if [ "$NO_CONFIG" -eq 0 ] && [ -f "$CBINDGEN_TOML" ]; then
  CONFIG_ARG="--config $CBINDGEN_TOML"
fi

# -----------------------------------------------------------------------------
# Crate list
# -----------------------------------------------------------------------------
#
# Format per entry (4 fields):
#   crate_name | crate_dir (relative to rust/crates) | out_header_name | lang
#
# Add more entries here if you later expose more FFI crates.
CRATES="
vitte_rust_api|vitte_rust_api|vitte_rust_api.h|C
"

# -----------------------------------------------------------------------------
# Generator
# -----------------------------------------------------------------------------

gen_one() {
  CRATE_NAME="$1"
  CRATE_SUBDIR="$2"
  OUT_NAME="$3"
  LANG="$4"

  CRATE_DIR="$RUST_DIR/crates/$CRATE_SUBDIR"
  MANIFEST="$CRATE_DIR/Cargo.toml"
  OUT_H="$OUT_DIR/$OUT_NAME"
  TMP_H="$TMP_DIR/$OUT_NAME"

  [ -f "$MANIFEST" ] || die "missing Cargo.toml for $CRATE_NAME at $MANIFEST"

  log "==> crate=$CRATE_NAME dir=$CRATE_DIR out=$OUT_H lang=$LANG"
  log "    manifest=$MANIFEST"
  if [ -n "$CONFIG_ARG" ]; then
    log "    config=$CBINDGEN_TOML"
  else
    log "    config=(none)"
  fi

  # Generate into temp file first.
  # shellcheck disable=SC2086
  cbindgen \
    --crate "$CRATE_NAME" \
    --manifest-path "$MANIFEST" \
    --output "$TMP_H" \
    --lang "$LANG" \
    $CONFIG_ARG

  [ -s "$TMP_H" ] || die "cbindgen produced empty header: $TMP_H"

  if [ "$CHECK" -eq 1 ]; then
    if [ ! -f "$OUT_H" ]; then
      echo "changed: $OUT_H (missing)" >&2
      exit 1
    fi
    if ! diff -u "$OUT_H" "$TMP_H" >/dev/null 2>&1; then
      echo "changed: $OUT_H" >&2
      diff -u "$OUT_H" "$TMP_H" || true
      exit 1
    fi
    log "ok: $OUT_H (no changes)"
    rm -f "$TMP_H"
    return 0
  fi

  # Overwrite only if content changed (reduces rebuild churn)
  if [ -f "$OUT_H" ] && diff -u "$OUT_H" "$TMP_H" >/dev/null 2>&1; then
    log "unchanged: $OUT_H"
    rm -f "$TMP_H"
    return 0
  fi

  mv "$TMP_H" "$OUT_H"
  log "written: $OUT_H"
}

# -----------------------------------------------------------------------------
# Run
# -----------------------------------------------------------------------------

FAILED=0

echo "$CRATES" | while IFS='|' read -r crate_name crate_subdir out_name lang; do
  # skip empty lines
  [ -n "${crate_name:-}" ] || continue

  # run
  if ! gen_one "$crate_name" "$crate_subdir" "$out_name" "$lang"; then
    FAILED=1
    break
  fi
done

rm -rf "$TMP_DIR"

if [ "${FAILED:-0}" -ne 0 ]; then
  exit 1
fi

if [ "$VERBOSE" -eq 1 ]; then
  echo "==> headers in: $OUT_DIR"
fi
