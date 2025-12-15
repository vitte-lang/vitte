#!/usr/bin/env sh
set -eu
rm -rf target || true

#!/usr/bin/env sh
#
# steel_clean.sh
#
# Workspace clean script for Steel/Vitte.
#
# Goals:
#   - Remove generated artifacts under ./target/ safely.
#   - Offer finer-grained cleaning (plan, bench, obj, bin, caches).
#   - Remain POSIX sh compatible (BSD/Solaris/Linux/macOS).
#
# Philosophy:
#   - Default behavior is conservative: clean ./target/ only.
#   - Never delete outside the workspace root.
#   - Dry-run supported for inspection.
#
# Usage:
#   ./build/scripts/steel_clean.sh
#   ./build/scripts/steel_clean.sh --all
#   ./build/scripts/steel_clean.sh --bench --plan
#   ./build/scripts/steel_clean.sh --dist
#   ./build/scripts/steel_clean.sh --dry-run --verbose
#

set -eu

# ------------------------------- helpers ---------------------------------

say() { printf "%s\n" "$*"; }
warn() { printf "steel_clean: %s\n" "$*" >&2; }
die() { printf "steel_clean: %s\n" "$*" >&2; exit 2; }

abspath() {
  _p="$1"
  if [ -z "$_p" ]; then return 1; fi
  if [ -d "$_p" ]; then
    (cd "$_p" 2>/dev/null && pwd)
  else
    _d=$(dirname "$_p")
    _b=$(basename "$_p")
    (cd "$_d" 2>/dev/null && printf "%s/%s\n" "$(pwd)" "$_b")
  fi
}

is_cmd() { command -v "$1" >/dev/null 2>&1; }

rm_rf() {
  _p="$1"
  [ -n "$_p" ] || return 0
  if [ -e "$_p" ]; then
    if [ "$DRY_RUN" -eq 1 ]; then
      say "DRY  rm -rf $_p"
    else
      rm -rf "$_p"
    fi
  fi
}

rm_f() {
  _p="$1"
  [ -n "$_p" ] || return 0
  if [ -f "$_p" ]; then
    if [ "$DRY_RUN" -eq 1 ]; then
      say "DRY  rm -f $_p"
    else
      rm -f "$_p"
    fi
  fi
}

# Remove children of a directory but keep the directory itself.
rm_dir_contents() {
  _d="$1"
  [ -n "$_d" ] || return 0
  [ -d "$_d" ] || return 0

  if [ "$DRY_RUN" -eq 1 ]; then
    say "DRY  rm -rf $_d/*"
    return 0
  fi

  # Portable approach: remove entries one by one.
  # shellcheck disable=SC2039
  for _e in "$_d"/* "$_d"/.*; do
    case "$_e" in
      "$_d/."|"$_d/..") continue ;;
    esac
    [ -e "$_e" ] || continue
    rm -rf "$_e"
  done
}

# Ensure the path is inside ROOT_DIR (prefix check).
inside_root() {
  _p="$1"
  case "$_p" in
    "$ROOT_DIR"/*|"$ROOT_DIR") return 0 ;;
    *) return 1 ;;
  esac
}

safe_rm_rf() {
  _p="$1"
  _ap=$(abspath "$_p" 2>/dev/null || echo "")
  [ -n "$_ap" ] || { warn "skip (cannot resolve): $_p"; return 0; }
  inside_root "$_ap" || die "refusing to delete outside workspace: $_ap"
  rm_rf "$_ap"
}

safe_rm_dir_contents() {
  _p="$1"
  _ap=$(abspath "$_p" 2>/dev/null || echo "")
  [ -n "$_ap" ] || { warn "skip (cannot resolve): $_p"; return 0; }
  inside_root "$_ap" || die "refusing to delete outside workspace: $_ap"
  rm_dir_contents "$_ap"
}

usage() {
  cat <<EOF
Usage: steel_clean.sh [options]

Default:
  Cleans ./target/

Options:
  --all           clean target/ plus generated lock/cache files
  --target        clean ./target/ (default)
  --obj           clean target/obj and target/bench/obj
  --bin           clean target/bin
  --plan          clean target/plan
  --bench         clean target/bench and target/bench_fs
  --cache         clean compiler/muffin caches (target/cache, target/muf_cache)
  --dist          clean packaging outputs (target/dist, target/pkg)
  --lock          remove muffin.lock (generated)

  --keep-dir      keep ./target/ directory, remove contents only
  --dry-run       print what would be deleted
  --verbose       extra logs
  -h, --help      show help
EOF
}

# ------------------------------- config ----------------------------------

ROOT_DIR=$(abspath "$(dirname "$0")/../..") || exit 1
TARGET_DIR="$ROOT_DIR/target"

# Flags
DO_TARGET=0
DO_OBJ=0
DO_BIN=0
DO_PLAN=0
DO_BENCH=0
DO_CACHE=0
DO_DIST=0
DO_LOCK=0
KEEP_DIR=0
DRY_RUN=0
VERBOSE=0

# ----------------------------- parse args --------------------------------

if [ $# -eq 0 ]; then
  DO_TARGET=1
fi

while [ $# -gt 0 ]; do
  case "$1" in
    --all)
      DO_TARGET=1
      DO_OBJ=1
      DO_BIN=1
      DO_PLAN=1
      DO_BENCH=1
      DO_CACHE=1
      DO_DIST=1
      DO_LOCK=1
      shift
      ;;
    --target) DO_TARGET=1; shift ;;
    --obj) DO_OBJ=1; shift ;;
    --bin) DO_BIN=1; shift ;;
    --plan) DO_PLAN=1; shift ;;
    --bench) DO_BENCH=1; shift ;;
    --cache) DO_CACHE=1; shift ;;
    --dist) DO_DIST=1; shift ;;
    --lock) DO_LOCK=1; shift ;;

    --keep-dir) KEEP_DIR=1; shift ;;
    --dry-run) DRY_RUN=1; shift ;;
    --verbose) VERBOSE=1; shift ;;

    -h|--help) usage; exit 0 ;;
    *) usage >&2; exit 1 ;;
  esac
done

# ----------------------------- preflight ---------------------------------

if [ ! -f "$ROOT_DIR/muffin.muf" ]; then
  warn "missing muffin.muf at workspace root; continuing anyway"
fi

if [ "$VERBOSE" -eq 1 ]; then
  say "steel_clean: root=$ROOT_DIR"
  say "steel_clean: target=$TARGET_DIR"
  say "steel_clean: dry_run=$DRY_RUN keep_dir=$KEEP_DIR"
fi

# If user asked for sub-clean without explicit --target, keep target dir.
# But if --target alone is set (default), we follow KEEP_DIR flag.

# ------------------------------ actions ----------------------------------

# Convenience: if only --obj/--bin/--plan/--bench/--cache/--dist were specified,
# do not delete full target/ unless requested.

if [ "$DO_TARGET" -eq 1 ]; then
  if [ -d "$TARGET_DIR" ]; then
    if [ "$KEEP_DIR" -eq 1 ]; then
      if [ "$VERBOSE" -eq 1 ]; then say "steel_clean: clearing target/ contents"; fi
      safe_rm_dir_contents "$TARGET_DIR"
    else
      if [ "$VERBOSE" -eq 1 ]; then say "steel_clean: removing target/"; fi
      safe_rm_rf "$TARGET_DIR"
    fi
  else
    [ "$VERBOSE" -eq 0 ] || say "steel_clean: target/ does not exist"
  fi
fi

# If target/ was removed entirely, remaining actions are no-ops.

if [ "$DO_OBJ" -eq 1 ]; then
  safe_rm_rf "$TARGET_DIR/obj"
  safe_rm_rf "$TARGET_DIR/bench/obj"
fi

if [ "$DO_BIN" -eq 1 ]; then
  safe_rm_rf "$TARGET_DIR/bin"
fi

if [ "$DO_PLAN" -eq 1 ]; then
  safe_rm_rf "$TARGET_DIR/plan"
fi

if [ "$DO_BENCH" -eq 1 ]; then
  safe_rm_rf "$TARGET_DIR/bench"
  safe_rm_rf "$TARGET_DIR/bench_fs"
fi

if [ "$DO_CACHE" -eq 1 ]; then
  safe_rm_rf "$TARGET_DIR/cache"
  safe_rm_rf "$TARGET_DIR/muf_cache"
  safe_rm_rf "$TARGET_DIR/vitte_cache"
fi

if [ "$DO_DIST" -eq 1 ]; then
  safe_rm_rf "$TARGET_DIR/dist"
  safe_rm_rf "$TARGET_DIR/pkg"
  safe_rm_rf "$TARGET_DIR/releases"
fi

if [ "$DO_LOCK" -eq 1 ]; then
  rm_f "$ROOT_DIR/muffin.lock"
fi

if [ "$VERBOSE" -eq 1 ]; then
  say "steel_clean: done"
fi

exit 0