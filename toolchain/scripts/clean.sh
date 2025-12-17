#!/usr/bin/env sh
# toolchain/scripts/clean.sh
#
# Vitte toolchain – clean build artifacts (portable POSIX sh).
#
# Usage:
#   ./toolchain/scripts/clean.sh
#   ./toolchain/scripts/clean.sh --all
#   ./toolchain/scripts/clean.sh --dist
#   ./toolchain/scripts/clean.sh --cache
#   ./toolchain/scripts/clean.sh --dry-run
#
# Notes:
#  - Designed to be safe: only removes known build/artifact directories/files.
#  - Works on macOS/Linux; on Windows use Git-Bash/MSYS2/WSL.

set -eu

ROOT_DIR="${ROOT_DIR:-$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)}"

DRY_RUN=0
ALL=0
DIST=0
CACHE=0
VERBOSE=0

usage() {
  cat <<'EOF'
clean.sh – remove Vitte build/toolchain artifacts

Options:
  --all       Remove build + dist + caches + generated toolchain outputs
  --dist      Remove distribution artifacts only
  --cache     Remove caches only
  --dry-run   Print what would be removed without deleting
  -v          Verbose
  -h, --help  Show help

Environment:
  ROOT_DIR    Override repository root (auto-detected by default)
EOF
}

log() {
  [ "$VERBOSE" -eq 1 ] && printf '%s\n' "$*"
}

rm_path() {
  p="$1"
  [ -z "$p" ] && return 0

  # Refuse dangerous patterns
  case "$p" in
    "/"|"/*"|"$HOME"|"$HOME/"* ) printf 'refuse to remove dangerous path: %s\n' "$p" >&2; exit 2 ;;
  esac

  if [ "$DRY_RUN" -eq 1 ]; then
    printf '[dry-run] rm -rf %s\n' "$p"
    return 0
  fi

  if [ -e "$p" ]; then
    log "rm -rf $p"
    rm -rf -- "$p"
  fi
}

rm_file_glob() {
  dir="$1"
  pat="$2"
  [ -z "$dir" ] && return 0
  [ -z "$pat" ] && return 0

  if [ ! -d "$dir" ]; then
    return 0
  fi

  # shellcheck disable=SC2039
  for f in "$dir"/$pat; do
    [ -e "$f" ] || continue
    if [ "$DRY_RUN" -eq 1 ]; then
      printf '[dry-run] rm -f %s\n' "$f"
    else
      log "rm -f $f"
      rm -f -- "$f"
    fi
  done
}

while [ $# -gt 0 ]; do
  case "$1" in
    --all) ALL=1 ;;
    --dist) DIST=1 ;;
    --cache) CACHE=1 ;;
    --dry-run) DRY_RUN=1 ;;
    -v) VERBOSE=1 ;;
    -h|--help) usage; exit 0 ;;
    *)
      printf 'unknown option: %s\n\n' "$1" >&2
      usage
      exit 2
      ;;
  esac
  shift
done

# Default behavior: clean build artifacts only.
if [ "$ALL" -eq 0 ] && [ "$DIST" -eq 0 ] && [ "$CACHE" -eq 0 ]; then
  # build only
  :
fi

log "ROOT_DIR=$ROOT_DIR"

# -----------------------------------------------------------------------------
# Build artifacts (repo-wide common names)
# -----------------------------------------------------------------------------

if [ "$ALL" -eq 1 ] || { [ "$DIST" -eq 0 ] && [ "$CACHE" -eq 0 ]; }; then
  rm_path "$ROOT_DIR/build"
  rm_path "$ROOT_DIR/out"
  rm_path "$ROOT_DIR/.cache"
  rm_path "$ROOT_DIR/.tmp"
  rm_path "$ROOT_DIR/.work"

  # Toolchain-specific generated folders (if used by your driver)
  rm_path "$ROOT_DIR/toolchain/build"
  rm_path "$ROOT_DIR/toolchain/out"
  rm_path "$ROOT_DIR/toolchain/.cache"
  rm_path "$ROOT_DIR/toolchain/.tmp"

  # Common CMake outputs if present
  rm_path "$ROOT_DIR/CMakeFiles"
  rm_file_glob "$ROOT_DIR" "CMakeCache.txt"
  rm_file_glob "$ROOT_DIR" "cmake_install.cmake"
  rm_file_glob "$ROOT_DIR" "compile_commands.json"

  # Ninja/Make residue
  rm_file_glob "$ROOT_DIR" "build.ninja"
  rm_file_glob "$ROOT_DIR" ".ninja_log"
  rm_file_glob "$ROOT_DIR" ".ninja_deps"
  rm_file_glob "$ROOT_DIR" "Makefile"
fi

# -----------------------------------------------------------------------------
# Toolchain caches / response files
# -----------------------------------------------------------------------------

if [ "$ALL" -eq 1 ] || [ "$CACHE" -eq 1 ]; then
  rm_path "$ROOT_DIR/build/.cache"
  rm_path "$ROOT_DIR/build/.rsp"
  rm_path "$ROOT_DIR/toolchain/build/.cache"
  rm_path "$ROOT_DIR/toolchain/build/.rsp"
fi

# -----------------------------------------------------------------------------
# Distribution artifacts
# -----------------------------------------------------------------------------

if [ "$ALL" -eq 1 ] || [ "$DIST" -eq 1 ]; then
  rm_path "$ROOT_DIR/dist"
  rm_path "$ROOT_DIR/release"
  rm_path "$ROOT_DIR/artifacts"
fi

# -----------------------------------------------------------------------------
# Generated object/deps (safe sweep in build trees)
# -----------------------------------------------------------------------------

if [ "$ALL" -eq 1 ] || { [ "$DIST" -eq 0 ] && [ "$CACHE" -eq 0 ]; }; then
  # Remove stray objects/deps in repo root (if any)
  rm_file_glob "$ROOT_DIR" "*.o"
  rm_file_glob "$ROOT_DIR" "*.obj"
  rm_file_glob "$ROOT_DIR" "*.a"
  rm_file_glob "$ROOT_DIR" "*.lib"
  rm_file_glob "$ROOT_DIR" "*.so"
  rm_file_glob "$ROOT_DIR" "*.dylib"
  rm_file_glob "$ROOT_DIR" "*.dll"
  rm_file_glob "$ROOT_DIR" "*.d"
  rm_file_glob "$ROOT_DIR" "*.pdb"
  rm_file_glob "$ROOT_DIR" "*.ilk"
fi

log "clean complete"
