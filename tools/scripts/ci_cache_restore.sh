#!/usr/bin/env sh
# C:\Users\vince\Documents\GitHub\vitte\tools\scripts\ci_cache_restore.sh
#
# CI cache restore helper for Vitte.
#
# Restores caches from a cache directory into the workspace to speed up builds.
#
# Recommended cache layout inside CacheDir:
#   build/                (optional, often large)
#   .cache/               (optional)
#   dist/                 (optional)
#   .muffin/ or muffin/   (optional)
#   rust/target/          (optional)
#
# Usage:
#   sh tools/scripts/ci_cache_restore.sh
#   sh tools/scripts/ci_cache_restore.sh --cache-dir .ci-cache --what tool,dist,muffin
#   sh tools/scripts/ci_cache_restore.sh --cache-dir "$RUNNER_TEMP/vitte-cache" --restore-build
#
# Exit codes:
#   0 ok
#   2 cache missing / nothing restored (only if --fail-if-empty)
#   3 bad args

set -eu

CACHE_DIR=".ci-cache"
WORKSPACE="."
WHAT="all"              # all | comma list: build,tool,dist,muffin,rust
RESTORE_BUILD=0
FAIL_IF_EMPTY=0

usage() {
  cat <<EOF
Usage: sh tools/scripts/ci_cache_restore.sh [options]
  --cache-dir DIR       cache directory (default: $CACHE_DIR)
  --workspace DIR       workspace root (default: .)
  --what LIST           all | build,tool,dist,muffin,rust (default: $WHAT)
  --restore-build       also restore build/ (can be large)
  --fail-if-empty       exit 2 if nothing restored
  -h, --help
EOF
}

die(){ printf "%s\n" "$*" 1>&2; exit 3; }

is_dir(){ [ -d "$1" ]; }
mk(){ mkdir -p "$1"; }

copy_dir() {
  src="$1"
  dst="$2"
  [ -d "$src" ] || return 1
  mk "$(dirname "$dst")"
  # Prefer rsync if available for speed; fallback to cp -a.
  if command -v rsync >/dev/null 2>&1; then
    rsync -a --delete "$src"/ "$dst"/
  else
    rm -rf "$dst"
    cp -a "$src" "$dst"
  fi
  return 0
}

# args
while [ $# -gt 0 ]; do
  case "$1" in
    --cache-dir) shift; [ $# -gt 0 ] || die "--cache-dir requires a value"; CACHE_DIR="$1" ;;
    --workspace) shift; [ $# -gt 0 ] || die "--workspace requires a value"; WORKSPACE="$1" ;;
    --what) shift; [ $# -gt 0 ] || die "--what requires a value"; WHAT="$1" ;;
    --restore-build) RESTORE_BUILD=1 ;;
    --fail-if-empty) FAIL_IF_EMPTY=1 ;;
    -h|--help) usage; exit 0 ;;
    *) die "Unknown arg: $1 (use --help)" ;;
  esac
  shift
done

if ! is_dir "$CACHE_DIR"; then
  msg="[cache] missing cache dir: $CACHE_DIR"
  if [ "$FAIL_IF_EMPTY" -eq 1 ]; then
    printf "%s\n" "$msg" 1>&2
    exit 2
  fi
  printf "%s\n" "$msg"
  exit 0
fi

# Normalize WHAT
if [ "$WHAT" = "all" ]; then
  ITEMS="build tool dist muffin rust"
else
  ITEMS="$(printf "%s" "$WHAT" | tr ',' ' ')"
fi

WS_ABS="$WORKSPACE"
CACHE_ABS="$CACHE_DIR"

printf "[cache] restore from=%s to=%s\n" "$CACHE_ABS" "$WS_ABS"

restored=""

for item in $ITEMS; do
  case "$item" in
    build)
      if [ "$RESTORE_BUILD" -ne 1 ]; then
        printf "[cache] skip build/ (--restore-build not set)\n"
        continue
      fi
      if copy_dir "$CACHE_ABS/build" "$WS_ABS/build"; then restored="$restored build"; fi
      ;;
    tool)
      if copy_dir "$CACHE_ABS/.cache" "$WS_ABS/.cache"; then restored="$restored .cache"; fi
      ;;
    dist)
      if copy_dir "$CACHE_ABS/dist" "$WS_ABS/dist"; then restored="$restored dist"; fi
      ;;
    muffin)
      ok=0
      if copy_dir "$CACHE_ABS/.muffin" "$WS_ABS/.muffin"; then restored="$restored .muffin"; ok=1; fi
      if copy_dir "$CACHE_ABS/muffin" "$WS_ABS/muffin"; then restored="$restored muffin"; ok=1; fi
      [ "$ok" -eq 1 ] || true
      ;;
    rust)
      if copy_dir "$CACHE_ABS/rust/target" "$WS_ABS/rust/target"; then restored="$restored rust/target"; fi
      ;;
    *)
      die "Unknown --what item: $item"
      ;;
  esac
done

# trim
restored="$(printf "%s" "$restored" | awk '{$1=$1;print}')"

if [ -z "$restored" ]; then
  msg="[cache] nothing restored"
  if [ "$FAIL_IF_EMPTY" -eq 1 ]; then
    printf "%s\n" "$msg" 1>&2
    exit 2
  fi
  printf "%s\n" "$msg"
  exit 0
fi

printf "[cache] restored: %s\n" "$restored"
exit 0
