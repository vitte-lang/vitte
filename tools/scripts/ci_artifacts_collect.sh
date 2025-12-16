#!/usr/bin/env sh
# tools/scripts/ci_artifacts_collect.sh
#
# CI artifact collector for Vitte.
# Collects:
#   - binaries (vittec)
#   - debug symbols (best-effort)
#   - logs (if present)
#   - checksums.sha256 (if present)
#   - selected metadata files (VERSION, LICENSE, muffin.muf, muffin.lock)
#
# Produces:
#   dist/artifacts/<stamp>/...
#   dist/artifacts/<stamp>.tar.gz (if tar available)
#
# Usage:
#   sh tools/scripts/ci_artifacts_collect.sh
#   sh tools/scripts/ci_artifacts_collect.sh --mode release --out dist/artifacts
#
# Exit codes:
#   0 ok
#   2 failed to collect (missing expected)
#   3 packaging failed

set -eu

MODE="release"           # release|debug|asan|relwithdebinfo
BUILD_ROOT="build"
OUT_ROOT="dist/artifacts"
BIN_NAME="vittec"
STAMP=""
INCLUDE_BUILD_DIR=0      # 1 to include compile_commands.json etc.

usage() {
  cat <<EOF
Usage: sh tools/scripts/ci_artifacts_collect.sh [options]
  --mode MODE          release|debug|asan|relwithdebinfo (default: $MODE)
  --build-root DIR     build root (default: $BUILD_ROOT)
  --out DIR            output root (default: $OUT_ROOT)
  --bin NAME           binary name (default: $BIN_NAME)
  --stamp STR          force stamp name (default: auto UTC)
  --include-build-dir  include selected build outputs (compile_commands.json, CTest logs)
  -h, --help
EOF
}

die(){ printf "%s\n" "$*" 1>&2; exit 2; }
has(){ command -v "$1" >/dev/null 2>&1; }

utc_stamp() {
  # YYYYMMDDTHHMMSSZ
  if has date; then
    date -u "+%Y%m%dT%H%M%SZ"
  else
    printf "unknown_stamp"
  fi
}

build_dir_for_mode() {
  case "$1" in
    release) printf "%s/release\n" "$BUILD_ROOT" ;;
    debug) printf "%s/debug\n" "$BUILD_ROOT" ;;
    asan) printf "%s/asan\n" "$BUILD_ROOT" ;;
    relwithdebinfo) printf "%s/relwithdebinfo\n" "$BUILD_ROOT" ;;
    *) die "Unknown mode: $1" ;;
  esac
}

resolve_bin() {
  _bdir="$1"
  if [ -x "$_bdir/$BIN_NAME" ]; then
    printf "%s/%s\n" "$_bdir" "$BIN_NAME"
    return 0
  fi
  if [ -x "$_bdir/$BIN_NAME.exe" ]; then
    printf "%s/%s.exe\n" "$_bdir" "$BIN_NAME"
    return 0
  fi
  return 1
}

copy_if_exists() {
  src="$1"
  dst="$2"
  if [ -e "$src" ]; then
    mkdir -p "$(dirname "$dst")"
    cp -a "$src" "$dst"
    return 0
  fi
  return 1
}

# args
while [ $# -gt 0 ]; do
  case "$1" in
    --mode) shift; [ $# -gt 0 ] || die "--mode requires a value"; MODE="$1" ;;
    --build-root) shift; [ $# -gt 0 ] || die "--build-root requires a value"; BUILD_ROOT="$1" ;;
    --out) shift; [ $# -gt 0 ] || die "--out requires a value"; OUT_ROOT="$1" ;;
    --bin) shift; [ $# -gt 0 ] || die "--bin requires a value"; BIN_NAME="$1" ;;
    --stamp) shift; [ $# -gt 0 ] || die "--stamp requires a value"; STAMP="$1" ;;
    --include-build-dir) INCLUDE_BUILD_DIR=1 ;;
    -h|--help) usage; exit 0 ;;
    *) die "Unknown arg: $1" ;;
  esac
  shift
done

[ -n "$STAMP" ] || STAMP="$(utc_stamp)"

BDIR="$(build_dir_for_mode "$MODE")"
DEST="$OUT_ROOT/$STAMP"

mkdir -p "$DEST"

printf "[collect] mode=%s build_dir=%s out=%s\n" "$MODE" "$BDIR" "$DEST"

# Metadata
copy_if_exists "VERSION"     "$DEST/meta/VERSION" || true
copy_if_exists "LICENSE"     "$DEST/meta/LICENSE" || true
copy_if_exists "muffin.muf"  "$DEST/meta/muffin.muf" || true
copy_if_exists "muffin.lock" "$DEST/meta/muffin.lock" || true
copy_if_exists ".editorconfig" "$DEST/meta/.editorconfig" || true
copy_if_exists "CMakeLists.txt" "$DEST/meta/CMakeLists.txt" || true
copy_if_exists "Makefile" "$DEST/meta/Makefile" || true
copy_if_exists "checksums.sha256" "$DEST/meta/checksums.sha256" || true

# Binary
BIN_PATH=""
if BIN_PATH="$(resolve_bin "$BDIR")"; then
  mkdir -p "$DEST/bin"
  cp -a "$BIN_PATH" "$DEST/bin/"
else
  die "Binary not found in $BDIR ($BIN_NAME). Build first."
fi

# Symbols / debug info (best-effort)
# - macOS: dsymutil produces .dSYM
# - Linux: objcopy --only-keep-debug
# - Windows: pdb (if MSVC) would already exist; best-effort copy
mkdir -p "$DEST/symbols"
if has dsymutil && [ -f "$BIN_PATH" ]; then
  if dsymutil "$BIN_PATH" -o "$DEST/symbols/$(basename "$BIN_PATH").dSYM" >/dev/null 2>&1; then
    printf "[collect] dsymutil OK\n"
  fi
fi

if has objcopy && [ -f "$BIN_PATH" ]; then
  if objcopy --only-keep-debug "$BIN_PATH" "$DEST/symbols/$(basename "$BIN_PATH").debug" >/dev/null 2>&1; then
    printf "[collect] objcopy debug OK\n"
  fi
fi

# Logs (best-effort)
copy_if_exists "$BDIR/Testing/Temporary/LastTest.log" "$DEST/logs/LastTest.log" || true
copy_if_exists "$BDIR/Testing/Temporary/LastTestsFailed.log" "$DEST/logs/LastTestsFailed.log" || true

# Optional build dir outputs
if [ "$INCLUDE_BUILD_DIR" -eq 1 ]; then
  copy_if_exists "$BDIR/compile_commands.json" "$DEST/build/compile_commands.json" || true
  copy_if_exists "$BDIR/CMakeCache.txt" "$DEST/build/CMakeCache.txt" || true
fi

# Create archive
ARCHIVE="$OUT_ROOT/$STAMP.tar.gz"
if has tar; then
  (cd "$OUT_ROOT" && tar -czf "$STAMP.tar.gz" "$STAMP") || exit 3
  printf "[collect] archive=%s\n" "$ARCHIVE"
else
  printf "[collect] tar not found; skipping archive\n"
fi

printf "[collect] done\n"
exit 0
