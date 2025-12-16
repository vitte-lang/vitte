#!/usr/bin/env sh
# C:\Users\vince\Documents\GitHub\vitte\tools\scripts\dist_tools.sh
#
# dist_tools.sh (max)
#
# Distribution packager for Vitte tools.
#
# Builds (unless --no-build) and packages:
#   - vittec
#   - optional extra tools if present (best-effort): vitte_fmt, vitte_lsp, etc.
#   - metadata: VERSION, LICENSE, README*, muffin.muf, muffin.lock, checksums.sha256
#   - optional: std/ (if --include-std)
#   - optional: symbols (best-effort: .dSYM/.debug)
#
# Produces:
#   dist/tools/<name>-<version>-<os>-<arch>/
#   dist/tools/<name>-<version>-<os>-<arch>.tar.gz (if tar available)
#
# Usage:
#   sh tools/scripts/dist_tools.sh
#   sh tools/scripts/dist_tools.sh --mode release --include-std --include-symbols
#   sh tools/scripts/dist_tools.sh --mode release --tools vittec,vitte_fmt --jobs 12 --force
#   sh tools/scripts/dist_tools.sh --mode debug --no-build
#
# Exit codes:
#   0 ok
#   2 build/configure/tooling error
#   3 packaging error

set -eu

MODE="release"          # release|debug|asan|relwithdebinfo
BUILD_ROOT="build"
OUT_ROOT="dist/tools"
PKG_NAME="vitte-tools"
TOOLS="auto"            # auto|comma list
NO_BUILD=0
INCLUDE_STD=0
INCLUDE_SYMBOLS=0
INCLUDE_BUILD_META=0
JOBS=""
FORCE=0

usage() {
  cat <<EOF
Usage: sh tools/scripts/dist_tools.sh [options]
  --mode MODE            release|debug|asan|relwithdebinfo (default: $MODE)
  --build-root DIR       build root (default: $BUILD_ROOT)
  --out-root DIR         output root (default: $OUT_ROOT)
  --package-name NAME    package prefix (default: $PKG_NAME)
  --tools LIST           auto | vittec,vitte_fmt,... (default: $TOOLS)
  --no-build             do not run cmake build
  --include-std          include std/ in package
  --include-symbols      include debug symbols (best-effort)
  --include-build-meta   include compile_commands.json, CMakeCache.txt (best-effort)
  -j, --jobs N           build parallelism
  --force                overwrite existing outputs
  -h, --help
EOF
}

die(){ printf "%s\n" "$*" 1>&2; exit 2; }
has(){ command -v "$1" >/dev/null 2>&1; }

os_norm() {
  u="$(uname -s 2>/dev/null || echo unknown)"
  case "$u" in
    Darwin) printf "macos\n" ;;
    Linux) printf "linux\n" ;;
    MINGW*|MSYS*|CYGWIN*) printf "windows\n" ;;
    *) printf "unknown\n" ;;
  esac
}

arch_norm() {
  a="$(uname -m 2>/dev/null || echo unknown)"
  case "$a" in
    x86_64|amd64) printf "x86_64\n" ;;
    arm64|aarch64) printf "aarch64\n" ;;
    i386|i686|x86) printf "x86\n" ;;
    *) printf "%s\n" "$a" ;;
  esac
}

get_version() {
  if [ -f "VERSION" ]; then
    v="$(tr -d '\r\n' < VERSION | awk '{$1=$1;print}')"
    [ -n "$v" ] && { printf "%s\n" "$v"; return; }
  fi
  printf "0.0.0\n"
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

cfg_for_mode() {
  case "$1" in
    release) printf "Release\n" ;;
    debug) printf "Debug\n" ;;
    relwithdebinfo) printf "RelWithDebInfo\n" ;;
    asan) printf "Debug\n" ;;
    *) die "Unknown mode: $1" ;;
  esac
}

san_for_mode() {
  case "$1" in
    asan) printf "ON\n" ;;
    *) printf "OFF\n" ;;
  esac
}

resolve_exe() {
  dir="$1"; name="$2"
  if [ -x "$dir/$name" ]; then printf "%s/%s\n" "$dir" "$name"; return 0; fi
  if [ -x "$dir/$name.exe" ]; then printf "%s/%s.exe\n" "$dir" "$name"; return 0; fi
  if [ -f "$dir/$name.exe" ]; then printf "%s/%s.exe\n" "$dir" "$name.exe"; return 0; fi
  return 1
}

copy_if_exists() {
  src="$1"; dst="$2"
  [ -e "$src" ] || return 1
  mkdir -p "$(dirname "$dst")"
  cp -a "$src" "$dst"
  return 0
}

cmake_configure_build() {
  bdir="$1"; cfg="$2"; san="$3"
  has cmake || die "cmake not found"
  GEN=""
  has ninja && GEN="-G Ninja"
  mkdir -p "$bdir"
  if [ ! -f "$bdir/CMakeCache.txt" ]; then
    cmake -S . -B "$bdir" $GEN \
      -DCMAKE_BUILD_TYPE="$cfg" \
      -DVITTE_BUILD_TESTS=OFF \
      -DVITTE_BUILD_BENCH=OFF \
      -DVITTE_ENABLE_SANITIZERS="$san" || exit 2
  fi
  if [ -n "$JOBS" ]; then
    cmake --build "$bdir" --config "$cfg" -- -j "$JOBS" || exit 2
  else
    cmake --build "$bdir" --config "$cfg" || exit 2
  fi
}

parse_tools() {
  if [ "$TOOLS" = "auto" ]; then
    # candidates: include those that exist in build dir
    candidates="vittec vitte_fmt vitte_lsp vitte_muf vitte_regex vitte_unicode"
    out=""
    for t in $candidates; do
      if resolve_exe "$BDIR" "$t" >/dev/null 2>&1; then
        out="$out $t"
      fi
    done
    out="$(printf "%s" "$out" | awk '{$1=$1;print}')"
    [ -n "$out" ] && { printf "%s\n" "$out"; return; }
    printf "vittec\n"
    return
  fi
  printf "%s\n" "$(printf "%s" "$TOOLS" | tr ',' ' ' | awk '{$1=$1;print}')"
}

collect_symbols_for_bin() {
  bin="$1"
  symdir="$2"
  mkdir -p "$symdir"
  ok=0

  # macOS: dsymutil -> .dSYM
  if [ "$(os_norm)" = "macos" ] && has dsymutil; then
    out="$symdir/$(basename "$bin").dSYM"
    if dsymutil "$bin" -o "$out" >/dev/null 2>&1; then ok=1; fi
  fi

  # Linux: objcopy --only-keep-debug
  if [ "$(os_norm)" = "linux" ] && has objcopy; then
    out="$symdir/$(basename "$bin").debug"
    if objcopy --only-keep-debug "$bin" "$out" >/dev/null 2>&1; then ok=1; fi
  fi

  return $ok
}

# args
while [ $# -gt 0 ]; do
  case "$1" in
    --mode) shift; [ $# -gt 0 ] || die "--mode requires a value"; MODE="$1" ;;
    --build-root) shift; [ $# -gt 0 ] || die "--build-root requires a value"; BUILD_ROOT="$1" ;;
    --out-root) shift; [ $# -gt 0 ] || die "--out-root requires a value"; OUT_ROOT="$1" ;;
    --package-name) shift; [ $# -gt 0 ] || die "--package-name requires a value"; PKG_NAME="$1" ;;
    --tools) shift; [ $# -gt 0 ] || die "--tools requires a value"; TOOLS="$1" ;;
    --no-build) NO_BUILD=1 ;;
    --include-std) INCLUDE_STD=1 ;;
    --include-symbols) INCLUDE_SYMBOLS=1 ;;
    --include-build-meta) INCLUDE_BUILD_META=1 ;;
    -j|--jobs) shift; [ $# -gt 0 ] || die "--jobs requires a value"; JOBS="$1" ;;
    --force) FORCE=1 ;;
    -h|--help) usage; exit 0 ;;
    *) die "Unknown arg: $1" ;;
  esac
  shift
done

OS="$(os_norm)"
ARCH="$(arch_norm)"
VER="$(get_version)"

BDIR="$(build_dir_for_mode "$MODE")"
CFG="$(cfg_for_mode "$MODE")"
SAN="$(san_for_mode "$MODE")"

if [ "$NO_BUILD" -eq 0 ]; then
  cmake_configure_build "$BDIR" "$CFG" "$SAN"
fi

TOOL_LIST="$(parse_tools)"
PKG_DIR_NAME="${PKG_NAME}-${VER}-${OS}-${ARCH}"
PKG_DIR="${OUT_ROOT}/${PKG_DIR_NAME}"
ARCHIVE="${OUT_ROOT}/${PKG_DIR_NAME}.tar.gz"

mkdir -p "$OUT_ROOT"

if [ "$FORCE" -eq 1 ]; then
  rm -rf "$PKG_DIR" "$ARCHIVE" 2>/dev/null || true
fi

mkdir -p "$PKG_DIR/bin" "$PKG_DIR/meta"

printf "[dist] mode=%s build_dir=%s\n" "$MODE" "$BDIR"
printf "[dist] package=%s\n" "$PKG_DIR_NAME"
printf "[dist] tools=%s\n" "$TOOL_LIST"

# copy tools
for t in $TOOL_LIST; do
  if exe="$(resolve_exe "$BDIR" "$t")"; then
    cp -a "$exe" "$PKG_DIR/bin/"
  else
    printf "%s\n" "Missing tool binary in $BDIR: $t" 1>&2
    exit 3
  fi
done

# meta
copy_if_exists "VERSION"        "$PKG_DIR/meta/VERSION"        || true
copy_if_exists "LICENSE"        "$PKG_DIR/meta/LICENSE"        || true
copy_if_exists "muffin.muf"     "$PKG_DIR/meta/muffin.muf"     || true
copy_if_exists "muffin.lock"    "$PKG_DIR/meta/muffin.lock"    || true
copy_if_exists "checksums.sha256" "$PKG_DIR/meta/checksums.sha256" || true
for r in README README.md README.txt; do
  [ -f "$r" ] && copy_if_exists "$r" "$PKG_DIR/meta/$r" || true
done

# optional std/
if [ "$INCLUDE_STD" -eq 1 ] && [ -d "std" ]; then
  cp -a "std" "$PKG_DIR/std"
fi

# optional build metadata
if [ "$INCLUDE_BUILD_META" -eq 1 ]; then
  copy_if_exists "$BDIR/compile_commands.json" "$PKG_DIR/meta/build/compile_commands.json" || true
  copy_if_exists "$BDIR/CMakeCache.txt"        "$PKG_DIR/meta/build/CMakeCache.txt"        || true
fi

# optional symbols
if [ "$INCLUDE_SYMBOLS" -eq 1 ]; then
  mkdir -p "$PKG_DIR/symbols"
  for t in $TOOL_LIST; do
    if exe="$(resolve_exe "$BDIR" "$t")"; then
      collect_symbols_for_bin "$exe" "$PKG_DIR/symbols" || true
    fi
  done
fi

# manifest
{
  printf "package=%s\n" "$PKG_DIR_NAME"
  printf "version=%s\n" "$VER"
  printf "os=%s\n" "$OS"
  printf "arch=%s\n" "$ARCH"
  printf "mode=%s\n" "$MODE"
  printf "tools=%s\n" "$TOOL_LIST"
  printf "timestamp_utc=%s\n" "$(date -u "+%Y-%m-%dT%H:%M:%SZ" 2>/dev/null || echo unknown)"
} > "$PKG_DIR/manifest.txt"

# archive
if has tar; then
  (cd "$OUT_ROOT" && tar -czf "${PKG_DIR_NAME}.tar.gz" "$PKG_DIR_NAME") || exit 3
else
  printf "[dist] tar not found; skipping archive\n"
fi

printf "[dist] outputs:\n"
printf "  dir = %s\n" "$PKG_DIR"
[ -f "$ARCHIVE" ] && printf "  tgz = %s\n" "$ARCHIVE" || true

exit 0
