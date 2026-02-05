#!/usr/bin/env bash
# ============================================================
# vitte — developer vittec runner
# Location: toolchain/scripts/dev/run-vittec.sh
# ============================================================

set -euo pipefail

# ----------------------------
# Configuration
# ----------------------------
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"
TARGET_DIR="${TARGET_DIR:-$ROOT_DIR/target}"

VITTEC_BIN="${VITTEC_BIN:-$TARGET_DIR/bin/vittec}"
PROFILE="${PROFILE:-debug}"        # debug | release
MODE="${MODE:-run}"                # run | check | build | ast | emit
VERBOSE="${VERBOSE:-0}"             # 0 | 1
DRY_RUN="${DRY_RUN:-0}"             # 0 | 1

# Inputs / Outputs
SRC="${SRC:-}"                      # .vit / .vitte
OUT_DIR="${OUT_DIR:-$TARGET_DIR/out}"
OUT_NAME="${OUT_NAME:-a.out}"

# Extra flags (passed verbatim)
VITTEC_FLAGS="${VITTEC_FLAGS:-}"

# ----------------------------
# Helpers
# ----------------------------
log() { printf "[vittec] %s\n" "$*"; }
die() { printf "[vittec][error] %s\n" "$*" >&2; exit 1; }

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

[ -x "$VITTEC_BIN" ] || die "vittec not found or not executable: $VITTEC_BIN"

if [ -z "$SRC" ]; then
  die "SRC not set (expected a .vit/.vitte file)"
fi

[ -f "$SRC" ] || die "source file not found: $SRC"

mkdir -p "$OUT_DIR"

OUT_PATH="$OUT_DIR/$OUT_NAME"

log "root=$ROOT_DIR"
log "profile=$PROFILE"
log "mode=$MODE"
log "src=$SRC"
log "out=$OUT_PATH"

# ----------------------------
# Build command
# ----------------------------
CMD=( "$VITTEC_BIN" )

case "$MODE" in
  run)
    CMD+=( run "$SRC" )
    ;;
  check)
    CMD+=( check "$SRC" )
    ;;
  build)
    CMD+=( build "$SRC" "--profile" "$PROFILE" "--output" "$OUT_PATH" )
    ;;
  ast)
    CMD+=( ast "$SRC" )
    ;;
  emit)
    CMD+=( emit "$SRC" "--profile" "$PROFILE" "--output" "$OUT_PATH" )
    ;;
  *)
    die "unknown MODE: $MODE"
    ;;
esac

if [ "$VERBOSE" = "1" ]; then
  CMD+=( "--verbose" )
fi

if [ -n "$VITTEC_FLAGS" ]; then
  # shellcheck disable=SC2206
  CMD+=( $VITTEC_FLAGS )
fi

# ----------------------------
# Run
# ----------------------------
run "${CMD[@]}"

log "done"
if [ "$MODE" = "build" ] || [ "$MODE" = "emit" ]; then
  echo "$OUT_PATH"
fi