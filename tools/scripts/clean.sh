#!/usr/bin/env sh
# C:\Users\vince\Documents\GitHub\vitte\tools\scripts\clean.sh
#
# Clean workspace artifacts (build/, dist/, caches, temp files).
#
# Usage:
#   sh tools/scripts/clean.sh              # safe default
#   sh tools/scripts/clean.sh --all        # also removes .cache/.ci-cache/.muffin (if present)
#   sh tools/scripts/clean.sh --build      # only build/
#   sh tools/scripts/clean.sh --dist       # only dist/
#   sh tools/scripts/clean.sh --dry-run    # show what would be removed
#
# Exit codes:
#   0 ok
#   2 bad args

set -eu

DRY=0
DO_BUILD=1
DO_DIST=1
DO_TMP=1
DO_ALL=0

usage() {
  cat <<EOF
Usage: sh tools/scripts/clean.sh [options]
  --build        remove build/ only
  --dist         remove dist/ only
  --tmp          remove temp/log artifacts only
  --all          also remove caches (.cache, .ci-cache, .muffin, muffin/, rust/target)
  --dry-run      print only (no deletion)
  -h, --help
EOF
}

die(){ printf "%s\n" "$*" 1>&2; exit 2; }

rm_path() {
  p="$1"
  if [ "$DRY" -eq 1 ]; then
    printf "[dry] rm -rf %s\n" "$p"
    return 0
  fi
  rm -rf "$p"
}

# args
if [ $# -gt 0 ]; then
  # if any scope flag is provided, default scopes become off and are enabled explicitly
  scoped=0
  for a in "$@"; do
    case "$a" in
      --build|--dist|--tmp) scoped=1 ;;
    esac
  done
  if [ "$scoped" -eq 1 ]; then
    DO_BUILD=0; DO_DIST=0; DO_TMP=0
  fi
fi

while [ $# -gt 0 ]; do
  case "$1" in
    --build) DO_BUILD=1 ;;
    --dist) DO_DIST=1 ;;
    --tmp) DO_TMP=1 ;;
    --all) DO_ALL=1 ;;
    --dry-run) DRY=1 ;;
    -h|--help) usage; exit 0 ;;
    *) die "Unknown arg: $1" ;;
  esac
  shift
done

printf "[clean] build=%s dist=%s tmp=%s all=%s dry=%s\n" "$DO_BUILD" "$DO_DIST" "$DO_TMP" "$DO_ALL" "$DRY"

# build/
if [ "$DO_BUILD" -eq 1 ] && [ -d "build" ]; then
  rm_path "build"
fi

# dist/
if [ "$DO_DIST" -eq 1 ] && [ -d "dist" ]; then
  rm_path "dist"
fi

# temp/log files
if [ "$DO_TMP" -eq 1 ]; then
  # common CI/test leftovers
  [ -f "checksums.sha256" ] && rm_path "checksums.sha256"
  [ -f "compile_commands.json" ] && rm_path "compile_commands.json" || true
  # prune CMake/CTest residue outside build/ (rare)
  [ -d "Testing" ] && rm_path "Testing" || true
  [ -d "CMakeFiles" ] && rm_path "CMakeFiles" || true
  [ -f "CMakeCache.txt" ] && rm_path "CMakeCache.txt" || true
fi

# caches (only with --all)
if [ "$DO_ALL" -eq 1 ]; then
  [ -d ".cache" ] && rm_path ".cache" || true
  [ -d ".ci-cache" ] && rm_path ".ci-cache" || true
  [ -d ".muffin" ] && rm_path ".muffin" || true
  [ -d "muffin" ] && rm_path "muffin" || true
  [ -d "rust/target" ] && rm_path "rust/target" || true
fi

printf "[clean] done\n"
exit 0
