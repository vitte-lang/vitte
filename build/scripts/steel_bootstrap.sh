#!/usr/bin/env sh
set -eu
# TODO: bootstrap toolchain

#!/usr/bin/env sh
#
# steel_bootstrap.sh
#
# Bootstrap builder for the Vitte toolchain (Steel).
#
# Goal:
#   - Produce host binaries:
#       * target/bin/steel
#       * target/bin/steelc
#   - Use the most hermetic toolchain available (zig cc) if present.
#   - Fall back to system C compiler (cc/clang/gcc).
#
# Design constraints:
#   - POSIX sh (no bashisms) for BSD/Solaris/Linux/macOS.
#   - No network required by default.
#   - Works with a flat C implementation (no staged compiler pipeline).
#
# Usage:
#   ./build/scripts/steel_bootstrap.sh
#   ./build/scripts/steel_bootstrap.sh --release
#   ./build/scripts/steel_bootstrap.sh --clean
#   ./build/scripts/steel_bootstrap.sh --cc clang
#   ./build/scripts/steel_bootstrap.sh --zig ./toolchain/zig/bin/zig
#
# Exit codes:
#   0 success
#   1 usage / configuration error
#   2 toolchain/build error

set -eu

# ------------------------------- helpers ---------------------------------

say() { printf "%s\n" "$*"; }
die() { printf "steel_bootstrap: %s\n" "$*" >&2; exit 2; }

# portable readlink-ish absolute path
abspath() {
  # usage: abspath path
  # best-effort: uses pwd + cd, avoids GNU readlink -f.
  _p="$1"
  if [ -z "$_p" ]; then
    return 1
  fi
  if [ -d "$_p" ]; then
    (cd "$_p" 2>/dev/null && pwd)
  else
    _d=$(dirname "$_p")
    _b=$(basename "$_p")
    (cd "$_d" 2>/dev/null && printf "%s/%s\n" "$(pwd)" "$_b")
  fi
}

is_cmd() { command -v "$1" >/dev/null 2>&1; }

mkdir_p() {
  # mkdir -p but with consistent errors
  _d="$1"
  [ -n "$_d" ] || return 0
  if [ ! -d "$_d" ]; then
    mkdir -p "$_d" || return 1
  fi
}

rm_rf() {
  _p="$1"
  [ -n "$_p" ] || return 0
  if [ -e "$_p" ]; then
    rm -rf "$_p"
  fi
}

uname_s() {
  # normalize OS name
  _u=$(uname -s 2>/dev/null || echo unknown)
  case "$_u" in
    Darwin) echo darwin ;;
    Linux) echo linux ;;
    FreeBSD) echo freebsd ;;
    NetBSD) echo netbsd ;;
    OpenBSD) echo openbsd ;;
    SunOS) echo solaris ;;
    *) echo "$_u" ;;
  esac
}

uname_m() {
  _m=$(uname -m 2>/dev/null || echo unknown)
  case "$_m" in
    x86_64|amd64) echo x86_64 ;;
    aarch64|arm64) echo aarch64 ;;
    *) echo "$_m" ;;
  esac
}

# ------------------------------- config ----------------------------------

ROOT_DIR=$(abspath "$(dirname "$0")/../..") || exit 1

MODE=debug
CLEAN=0
CC_OVERRIDE=""
ZIG_OVERRIDE=""
JOBS=""
QUIET=0

# Where to place build artifacts
TARGET_DIR="$ROOT_DIR/target"
OBJ_DIR="$TARGET_DIR/obj"
BIN_DIR="$TARGET_DIR/bin"

# Include roots
INC1="$ROOT_DIR/include"

# Default output
OUT_STEEL="$BIN_DIR/steel"
OUT_STEELC="$BIN_DIR/steelc"

usage() {
  cat <<EOF
Usage: steel_bootstrap.sh [options]

Options:
  --release            build with -O2 (default: debug)
  --debug              build with -O0 -g (default)
  --clean              remove target/ before building
  --cc <cc>            force C compiler (cc/clang/gcc)
  --zig <zig>          force zig path (uses: <zig> cc)
  -j, --jobs <n>       parallel compile jobs (default: auto where possible)
  --quiet              less output
  -h, --help           show this help
EOF
}

# ----------------------------- parse args --------------------------------

while [ $# -gt 0 ]; do
  case "$1" in
    --release) MODE=release; shift ;;
    --debug) MODE=debug; shift ;;
    --clean) CLEAN=1; shift ;;
    --cc) shift; [ $# -gt 0 ] || { usage >&2; exit 1; }; CC_OVERRIDE="$1"; shift ;;
    --zig) shift; [ $# -gt 0 ] || { usage >&2; exit 1; }; ZIG_OVERRIDE="$1"; shift ;;
    -j|--jobs) shift; [ $# -gt 0 ] || { usage >&2; exit 1; }; JOBS="$1"; shift ;;
    --quiet) QUIET=1; shift ;;
    -h|--help) usage; exit 0 ;;
    *) usage >&2; exit 1 ;;
  esac
done

# ----------------------------- environment -------------------------------

OS=$(uname_s)
ARCH=$(uname_m)

if [ "$QUIET" -ne 1 ]; then
  say "steel_bootstrap: root=$ROOT_DIR"
  say "steel_bootstrap: os=$OS arch=$ARCH mode=$MODE"
fi

# Verify workspace manifest exists (required by project philosophy)
if [ ! -f "$ROOT_DIR/muffin.muf" ]; then
  die "missing muffin.muf at workspace root: $ROOT_DIR/muffin.muf"
fi

if [ "$CLEAN" -eq 1 ]; then
  if [ "$QUIET" -ne 1 ]; then
    say "steel_bootstrap: cleaning $TARGET_DIR"
  fi
  rm_rf "$TARGET_DIR"
fi

mkdir_p "$OBJ_DIR"
mkdir_p "$BIN_DIR"

# ------------------------------ toolchain --------------------------------

# Prefer zig cc if present, because it simplifies cross-platform toolchain.
# If not present, fall back to system cc.

CC=""
CC_KIND=""

if [ -n "$ZIG_OVERRIDE" ]; then
  if [ ! -x "$ZIG_OVERRIDE" ]; then
    die "--zig provided but not executable: $ZIG_OVERRIDE"
  fi
  CC="$ZIG_OVERRIDE cc"
  CC_KIND="zig"
else
  if [ -x "$ROOT_DIR/toolchain/zig/bin/zig" ]; then
    CC="$ROOT_DIR/toolchain/zig/bin/zig cc"
    CC_KIND="zig"
  elif is_cmd zig; then
    CC="zig cc"
    CC_KIND="zig"
  else
    CC_KIND="system"
  fi
fi

if [ "$CC_KIND" = "system" ]; then
  if [ -n "$CC_OVERRIDE" ]; then
    CC="$CC_OVERRIDE"
  else
    if is_cmd cc; then CC=cc
    elif is_cmd clang; then CC=clang
    elif is_cmd gcc; then CC=gcc
    else die "no C compiler found (cc/clang/gcc) and zig not available"; fi
  fi
fi

# Validate compiler availability
# shellcheck disable=SC2086
if ! $CC --version >/dev/null 2>&1; then
  die "compiler not usable: $CC"
fi

if [ "$QUIET" -ne 1 ]; then
  say "steel_bootstrap: cc='$CC' (kind=$CC_KIND)"
fi

# Choose jobs
if [ -n "$JOBS" ]; then
  :
elif is_cmd getconf; then
  JOBS=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo "")
elif is_cmd sysctl; then
  JOBS=$(sysctl -n hw.ncpu 2>/dev/null || echo "")
else
  JOBS=""
fi

if [ -z "$JOBS" ]; then
  JOBS=1
fi

# ------------------------------ flags ------------------------------------

CFLAGS_COMMON="-I$INC1 -DSTEEL_BOOTSTRAP=1"

# Warnings: keep strict but practical.
CFLAGS_WARN="-Wall -Wextra -Wshadow -Wstrict-prototypes -Wmissing-prototypes"

# Platform defines
CFLAGS_OS=""
case "$OS" in
  darwin|linux|freebsd|netbsd|openbsd|solaris) CFLAGS_OS="$CFLAGS_OS -DSTEEL_POSIX=1" ;;
  *) CFLAGS_OS="$CFLAGS_OS" ;;
esac

# Mode flags
CFLAGS_MODE=""
LDFLAGS_MODE=""
if [ "$MODE" = "release" ]; then
  CFLAGS_MODE="-O2 -DNDEBUG=1"
else
  CFLAGS_MODE="-O0 -g -DDEBUG=1"
fi

# Optional: enable hardening features at compile-time where possible.
CFLAGS_HARDEN="-fno-omit-frame-pointer"

# Some compilers on BSD/Solaris need explicit std.
CFLAGS_STD="-std=c11"

# Assemble final compile flags
CFLAGS="$CFLAGS_STD $CFLAGS_COMMON $CFLAGS_WARN $CFLAGS_OS $CFLAGS_MODE $CFLAGS_HARDEN"

# Link flags
LDFLAGS="$LDFLAGS_MODE"

# On some platforms, you may want to add -lpthread explicitly.
# We do best-effort detection.
NEED_PTHREAD=0
case "$OS" in
  linux|freebsd|netbsd|openbsd|solaris) NEED_PTHREAD=1 ;;
  darwin) NEED_PTHREAD=0 ;;
esac

if [ "$NEED_PTHREAD" -eq 1 ]; then
  LDFLAGS="$LDFLAGS -lpthread"
fi

# ------------------------------ sources ----------------------------------

# Enumerate C sources explicitly by folders to keep predictable build.
# This mirrors the repository layout.

SRC_COMMON=
SRC_DIAG=
SRC_MUF=
SRC_VITTE=
SRC_COMPILER=
SRC_RUNTIME=
SRC_PAL=
SRC_CLI=

# Note: find(1) differs a bit across platforms. Use simple patterns.

find_c_sources() {
  _base="$1"
  if [ -d "$_base" ]; then
    find "$_base" -type f -name '*.c' 2>/dev/null | LC_ALL=C sort
  fi
}

SRC_COMMON=$(find_c_sources "$ROOT_DIR/src/common")
SRC_DIAG=$(find_c_sources "$ROOT_DIR/src/diag")
SRC_MUF=$(find_c_sources "$ROOT_DIR/src/muf")
SRC_VITTE=$(find_c_sources "$ROOT_DIR/src/vitte")
SRC_COMPILER=$(find_c_sources "$ROOT_DIR/src/compiler")
SRC_RUNTIME=$(find_c_sources "$ROOT_DIR/src/runtime")

# PAL: choose posix backend for sh-based build.
# Windows native build should use msvc + win32 sources.
if [ -d "$ROOT_DIR/src/pal/posix" ]; then
  SRC_PAL=$(find_c_sources "$ROOT_DIR/src/pal/posix")
elif [ -d "$ROOT_DIR/src/pal" ]; then
  SRC_PAL=$(find_c_sources "$ROOT_DIR/src/pal")
else
  SRC_PAL=""
fi

# CLI (two binaries share code; each has its own main)
SRC_CLI=$(find_c_sources "$ROOT_DIR/src/cli")

# Ensure expected mains exist
STEEL_MAIN="$ROOT_DIR/src/cli/steel_main.c"
STEELC_MAIN="$ROOT_DIR/src/cli/steelc_main.c"

if [ ! -f "$STEEL_MAIN" ]; then
  die "missing: $STEEL_MAIN"
fi
if [ ! -f "$STEELC_MAIN" ]; then
  die "missing: $STEELC_MAIN"
fi

# Remove mains from shared list; they will be linked separately.
filter_out_file() {
  _list="$1"
  _file="$2"
  echo "$_list" | awk -v f="$_file" '($0!=f){print $0}'
}

SRC_CLI_SHARED=$(filter_out_file "$SRC_CLI" "$STEEL_MAIN")
SRC_CLI_SHARED=$(filter_out_file "$SRC_CLI_SHARED" "$STEELC_MAIN")

# Aggregate library-like sources
ALL_LIB_SOURCES=""
if [ -n "$SRC_COMMON" ]; then ALL_LIB_SOURCES="$ALL_LIB_SOURCES\n$SRC_COMMON"; fi
if [ -n "$SRC_DIAG" ]; then ALL_LIB_SOURCES="$ALL_LIB_SOURCES\n$SRC_DIAG"; fi
if [ -n "$SRC_MUF" ]; then ALL_LIB_SOURCES="$ALL_LIB_SOURCES\n$SRC_MUF"; fi
if [ -n "$SRC_VITTE" ]; then ALL_LIB_SOURCES="$ALL_LIB_SOURCES\n$SRC_VITTE"; fi
if [ -n "$SRC_COMPILER" ]; then ALL_LIB_SOURCES="$ALL_LIB_SOURCES\n$SRC_COMPILER"; fi
if [ -n "$SRC_RUNTIME" ]; then ALL_LIB_SOURCES="$ALL_LIB_SOURCES\n$SRC_RUNTIME"; fi
if [ -n "$SRC_PAL" ]; then ALL_LIB_SOURCES="$ALL_LIB_SOURCES\n$SRC_PAL"; fi
if [ -n "$SRC_CLI_SHARED" ]; then ALL_LIB_SOURCES="$ALL_LIB_SOURCES\n$SRC_CLI_SHARED"; fi

# normalize list
ALL_LIB_SOURCES=$(printf "%s\n" "$ALL_LIB_SOURCES" | sed '/^$/d')

if [ -z "$ALL_LIB_SOURCES" ]; then
  die "no C sources found under src/ (project incomplete?)"
fi

# ------------------------------ build ------------------------------------

# Turn a source path into an object path under target/obj.
obj_for_src() {
  _src="$1"
  _rel=${_src#"$ROOT_DIR/"}
  _obj="$OBJ_DIR/${_rel%.c}.o"
  echo "$_obj"
}

compile_one() {
  _src="$1"
  _obj=$(obj_for_src "$_src")
  _od=$(dirname "$_obj")
  mkdir_p "$_od"

  # Rebuild if missing or if src newer than obj
  if [ ! -f "$_obj" ] || [ "$_src" -nt "$_obj" ]; then
    if [ "$QUIET" -ne 1 ]; then
      say "cc  ${_src#"$ROOT_DIR/"}"; fi
    # shellcheck disable=SC2086
    $CC $CFLAGS -c "$_src" -o "$_obj" || return 1
  fi
}

# Basic parallelism using xargs -P if available.
# On some platforms, xargs -P may not exist; fall back to sequential.

build_objects() {
  if is_cmd xargs && echo "x" | xargs -P 1 echo >/dev/null 2>&1; then
    # xargs -P supported
    printf "%s\n" "$ALL_LIB_SOURCES" \
      | xargs -P "$JOBS" -n 1 sh -c '
          set -eu
          ROOT_DIR="$1"; OBJ_DIR="$2"; CC="$3"; CFLAGS="$4"; QUIET="$5"
          src="$6"
          mkdir_p() { d="$1"; [ -n "$d" ] || exit 0; [ -d "$d" ] || mkdir -p "$d"; }
          obj_for_src() { s="$1"; rel=${s#"$ROOT_DIR/"}; echo "$OBJ_DIR/${rel%.c}.o"; }
          obj=$(obj_for_src "$src")
          od=$(dirname "$obj")
          mkdir_p "$od"
          if [ ! -f "$obj" ] || [ "$src" -nt "$obj" ]; then
            if [ "$QUIET" -ne 1 ]; then printf "cc  %s\n" "${src#"$ROOT_DIR/"}"; fi
            # shellcheck disable=SC2086
            $CC $CFLAGS -c "$src" -o "$obj"
          fi
        ' sh "$ROOT_DIR" "$OBJ_DIR" "$CC" "$CFLAGS" "$QUIET" || return 1
  else
    # sequential
    for src in $ALL_LIB_SOURCES; do
      compile_one "$src" || return 1
    done
  fi
}

collect_objects() {
  # prints object list to stdout
  for src in $ALL_LIB_SOURCES; do
    echo "$(obj_for_src "$src")"
  done
}

link_bin() {
  _out="$1"
  _main="$2"

  # compile main
  _main_obj=$(obj_for_src "$_main")
  mkdir_p "$(dirname "$_main_obj")"
  if [ ! -f "$_main_obj" ] || [ "$_main" -nt "$_main_obj" ]; then
    if [ "$QUIET" -ne 1 ]; then
      say "cc  ${_main#"$ROOT_DIR/"}"; fi
    # shellcheck disable=SC2086
    $CC $CFLAGS -c "$_main" -o "$_main_obj" || return 1
  fi

  _objs=$(collect_objects)

  if [ "$QUIET" -ne 1 ]; then
    say "ld  ${_out#"$ROOT_DIR/"}"; fi

  # shellcheck disable=SC2086
  $CC $_main_obj $_objs -o "$_out" $LDFLAGS || return 1
}

# ------------------------------ asm (optional) ---------------------------

# If you later wire asm objects into the build, prefer:
#   -DSTEEL_HAVE_ASM_MEMWIPE=1
#   -DSTEEL_HAVE_ASM_CT_EQ=1
# and add the relevant .S to compilation.
# This bootstrap script does not assemble by default to keep portability.

# ------------------------------ execute ----------------------------------

if [ "$QUIET" -ne 1 ]; then
  say "steel_bootstrap: compiling objects (jobs=$JOBS)"
fi

build_objects || die "compile failed"

link_bin "$OUT_STEEL" "$STEEL_MAIN" || die "link failed (steel)"
link_bin "$OUT_STEELC" "$STEELC_MAIN" || die "link failed (steelc)"

if [ "$QUIET" -ne 1 ]; then
  say "steel_bootstrap: ok"
  say "  $OUT_STEEL"
  say "  $OUT_STEELC"
fi

exit 0