#!/usr/bin/env bash
# ============================================================
# vitte — developer linker runner
# Location: toolchain/scripts/dev/run-linker.sh
# ============================================================

set -euo pipefail

# ----------------------------
# Configuration
# ----------------------------
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"
TARGET_DIR="${TARGET_DIR:-$ROOT_DIR/target}"
BUILD_KIND="${BUILD_KIND:-debug}"     # debug | release
VERBOSE="${VERBOSE:-0}"               # 0 | 1
DRY_RUN="${DRY_RUN:-0}"               # 0 | 1

# Linker inputs
LINKER_BIN="${LINKER_BIN:-$TARGET_DIR/bin/vitte-linker}"
OBJ_DIR="${OBJ_DIR:-$TARGET_DIR/obj}"
OUT_DIR="${OUT_DIR:-$TARGET_DIR/out}"
OUT_NAME="${OUT_NAME:-a.out}"

# Extra flags
LINK_FLAGS="${LINK_FLAGS:-}"

# ----------------------------
# Helpers
# ----------------------------
log() { printf "[linker] %s\n" "$*"; }
die() { printf "[linker][error] %s\n" "$*" >&2; exit 1; }

has() { command -v "$1" >/dev/null 2>&1; }

run() {
  if [ "$DRY_RUN" = "1" ]; then
    log "dry-run: $*"
  else
    if [ "$VERBOSE" = "1" ]; then
      log "exec: $*"
    fi
    "$@"
  fi
}

# ----------------------------
# Init
# ----------------------------
cd "$ROOT_DIR"

if [ ! -x "$LINKER_BIN" ]; then
  if [ -x "$ROOT_DIR/bin/vitte-linker" ]; then
    LINKER_BIN="$ROOT_DIR/bin/vitte-linker"
  else
    die "linker not found or not executable: $LINKER_BIN"
  fi
fi
[ -d "$OBJ_DIR" ] || die "object directory not found: $OBJ_DIR"

mkdir -p "$OUT_DIR"

OUT_PATH="$OUT_DIR/$OUT_NAME"

log "root=$ROOT_DIR"
log "build=$BUILD_KIND"
log "obj_dir=$OBJ_DIR"
log "out=$OUT_PATH"

# ----------------------------
# Collect objects
# ----------------------------
mapfile -t OBJECTS < <(find "$OBJ_DIR" -type f -name "*.o" | sort)

[ "${#OBJECTS[@]}" -gt 0 ] || die "no object files found in $OBJ_DIR"

if [ "$VERBOSE" = "1" ]; then
  log "objects:"
  for o in "${OBJECTS[@]}"; do
    log "  $o"
  done
fi

# ----------------------------
# Build command
# ----------------------------
CMD=(
  "$LINKER_BIN"
  "--build" "$BUILD_KIND"
  "--output" "$OUT_PATH"
)

for o in "${OBJECTS[@]}"; do
  CMD+=( "--object" "$o" )
done

if [ -n "$LINK_FLAGS" ]; then
  # shellcheck disable=SC2206
  CMD+=( $LINK_FLAGS )
fi

# ----------------------------
# Run
# ----------------------------
log "linking…"
run "${CMD[@]}"

log "link complete"
echo "$OUT_PATH"
