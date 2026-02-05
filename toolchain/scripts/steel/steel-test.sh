#!/usr/bin/env bash
# ============================================================
# vitte — Steel test wrapper
# Location: toolchain/scripts/steel/steel-test.sh
# ============================================================

set -euo pipefail

# ----------------------------
# Configuration
# ----------------------------
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"
STEEL_BIN="${STEEL_BIN:-steel}"              # steel CLI in PATH
STEEL_FILE="${STEEL_FILE:-steelconf}"        # steel config file
BAKE="${BAKE:-test}"                         # test bake target
PROFILE="${PROFILE:-debug}"                  # debug | release | custom
TARGET="${TARGET:-}"                         # optional target script
VERBOSE="${VERBOSE:-0}"                      # 0 | 1
DRY_RUN="${DRY_RUN:-0}"                      # 0 | 1
FAIL_FAST="${FAIL_FAST:-1}"                  # 1 | 0 (propagate first failure)

# CI metadata (optional)
CI_NAME="${CI_NAME:-local}"
CI_COMMIT="${CI_COMMIT:-$(git rev-parse --short HEAD 2>/dev/null || echo unknown)}"
CI_REF="${CI_REF:-$(git branch --show-current 2>/dev/null || echo unknown)}"

# ----------------------------
# Helpers
# ----------------------------
log() { printf "[steel-test] %s\n" "$*"; }
die() { printf "[steel-test][error] %s\n" "$*" >&2; exit 1; }

has() { command -v "$1" >/dev/null 2>&1; }

run() {
  if [ "$DRY_RUN" = "1" ]; then
    log "dry-run: $*"
  else
    if [ "$VERBOSE" = "1" ]; then log "exec: $*"; fi
    "$@"
  fi
}

# ----------------------------
# Init
# ----------------------------
cd "$ROOT_DIR"

has "$STEEL_BIN" || die "steel not found in PATH (set STEEL_BIN)"
[ -f "$STEEL_FILE" ] || die "steel file not found: $STEEL_FILE"

log "root=$ROOT_DIR"
log "steel=$STEEL_BIN file=$STEEL_FILE bake=$BAKE profile=$PROFILE"
log "ci=$CI_NAME commit=$CI_COMMIT ref=$CI_REF"

# ----------------------------
# Load target (optional)
# ----------------------------
if [ -n "$TARGET" ]; then
  TARGET_SCRIPT="$ROOT_DIR/toolchain/scripts/targets/$TARGET"
  [ -f "$TARGET_SCRIPT" ] || die "target script not found: $TARGET_SCRIPT"
  log "loading target: $TARGET"
  # shellcheck source=/dev/null
  source "$TARGET_SCRIPT"
fi

# ----------------------------
# Hooks (optional)
# ----------------------------
if declare -F vitte_target_pre_build >/dev/null 2>&1; then
  log "running target pre-test hook"
  vitte_target_pre_build
fi

# ----------------------------
# Run tests
# ----------------------------
CMD=(
  "$STEEL_BIN" run
  --root "$ROOT_DIR"
  --file "$STEEL_FILE"
  --bake "$BAKE"
  --profile "$PROFILE"
)

if [ "$VERBOSE" = "1" ]; then
  CMD+=( --verbose )
fi

if [ "$FAIL_FAST" = "1" ]; then
  CMD+=( --fail-fast )
fi

run "${CMD[@]}"

# ----------------------------
# Hooks (optional)
# ----------------------------
if declare -F vitte_target_post_build >/dev/null 2>&1; then
  log "running target post-test hook"
  vitte_target_post_build
fi

log "tests complete"