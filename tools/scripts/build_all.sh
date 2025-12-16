#!/usr/bin/env sh
# C:\Users\vince\Documents\GitHub\vitte\tools\scripts\build_all.sh
#
# Build everything (CMake presets: release/debug/asan, optional tests/bench).
# Portable: macOS/Linux/WSL/MSYS2 (best-effort).
#
# Usage:
#   sh tools/scripts/build_all.sh
#   sh tools/scripts/build_all.sh --mode all
#   sh tools/scripts/build_all.sh --mode release --tests on --bench off
#
# Exit codes:
#   0 ok
#   2 configure/build failed
#   3 tests failed

set -eu

MODE="all"       # all|release|debug|asan|relwithdebinfo
RUN_TESTS="off"  # on|off
RUN_BENCH="off"  # on|off
BUILD_ROOT="build"
JOBS=""

die() { printf "%s\n" "$*" 1>&2; exit 2; }
has() { command -v "$1" >/dev/null 2>&1; }

usage() {
  cat <<EOF
Usage: sh tools/scripts/build_all.sh [options]

Options:
  --mode MODE         all|release|debug|asan|relwithdebinfo (default: $MODE)
  --tests on|off      run ctest after builds (default: $RUN_TESTS)
  --bench on|off      build benchmarks (default: $RUN_BENCH)
  --build-root DIR    build root directory (default: $BUILD_ROOT)
  -j, --jobs N        parallel jobs (passed to cmake --build) (optional)
  -h, --help          show help
EOF
}

while [ $# -gt 0 ]; do
  case "$1" in
    --mode) shift; [ $# -gt 0 ] || die "--mode requires a value"; MODE="$1" ;;
    --tests) shift; [ $# -gt 0 ] || die "--tests requires a value"; RUN_TESTS="$1" ;;
    --bench) shift; [ $# -gt 0 ] || die "--bench requires a value"; RUN_BENCH="$1" ;;
    --build-root) shift; [ $# -gt 0 ] || die "--build-root requires a value"; BUILD_ROOT="$1" ;;
    -j|--jobs) shift; [ $# -gt 0 ] || die "--jobs requires a value"; JOBS="$1" ;;
    -h|--help) usage; exit 0 ;;
    *) die "Unknown arg: $1 (use --help)" ;;
  esac
  shift
done

[ "$RUN_TESTS" = "on" ] || [ "$RUN_TESTS" = "off" ] || die "--tests must be on|off"
[ "$RUN_BENCH" = "on" ] || [ "$RUN_BENCH" = "off" ] || die "--bench must be on|off"

if ! has cmake; then
  die "cmake not found in PATH."
fi

GENERATOR=""
if has ninja; then
  GENERATOR="-G Ninja"
fi

build_one() {
  _name="$1"
  _bdir="$2"
  _type="$3"
  _san="$4"      # ON|OFF
  _tests="$5"    # ON|OFF
  _bench="$6"    # ON|OFF

  printf "[build] %s: dir=%s type=%s sanitizers=%s tests=%s bench=%s\n" "$_name" "$_bdir" "$_type" "$_san" "$_tests" "$_bench"
  mkdir -p "$_bdir"

  cmake -S . -B "$_bdir" $GENERATOR \
    -DCMAKE_BUILD_TYPE="$_type" \
    -DVITTE_ENABLE_SANITIZERS="$_san" \
    -DVITTE_BUILD_TESTS="$_tests" \
    -DVITTE_BUILD_BENCH="$_bench"

  if [ -n "$JOBS" ]; then
    cmake --build "$_bdir" --config "$_type" -- -j "$JOBS"
  else
    cmake --build "$_bdir" --config "$_type"
  fi
}

run_ctest() {
  _bdir="$1"
  printf "[test] ctest in %s\n" "$_bdir"
  (cd "$_bdir" && ctest --output-on-failure)
}

# Determine what to build
DO_RELEASE=0
DO_DEBUG=0
DO_ASAN=0
DO_RELWITHDEBINFO=0

case "$MODE" in
  all) DO_RELEASE=1; DO_DEBUG=1 ;;
  release) DO_RELEASE=1 ;;
  debug) DO_DEBUG=1 ;;
  asan) DO_ASAN=1 ;;
  relwithdebinfo) DO_RELWITHDEBINFO=1 ;;
  *) die "Unknown mode: $MODE" ;;
esac

# Map on/off to ON/OFF for CMake
CMAKE_TESTS="OFF"
[ "$RUN_TESTS" = "on" ] && CMAKE_TESTS="ON"

CMAKE_BENCH="OFF"
[ "$RUN_BENCH" = "on" ] && CMAKE_BENCH="ON"

# Build requested configs
if [ "$DO_RELEASE" -eq 1 ]; then
  build_one "release" "$BUILD_ROOT/release" "Release" "OFF" "$CMAKE_TESTS" "$CMAKE_BENCH" || exit 2
fi

if [ "$DO_DEBUG" -eq 1 ]; then
  build_one "debug" "$BUILD_ROOT/debug" "Debug" "OFF" "$CMAKE_TESTS" "$CMAKE_BENCH" || exit 2
fi

if [ "$DO_ASAN" -eq 1 ]; then
  build_one "asan" "$BUILD_ROOT/asan" "Debug" "ON" "$CMAKE_TESTS" "$CMAKE_BENCH" || exit 2
fi

if [ "$DO_RELWITHDEBINFO" -eq 1 ]; then
  build_one "relwithdebinfo" "$BUILD_ROOT/relwithdebinfo" "RelWithDebInfo" "OFF" "$CMAKE_TESTS" "$CMAKE_BENCH" || exit 2
fi

# Run tests if requested (pick best dir: debug > asan > release)
if [ "$RUN_TESTS" = "on" ]; then
  if [ -d "$BUILD_ROOT/debug" ]; then
    run_ctest "$BUILD_ROOT/debug" || exit 3
  elif [ -d "$BUILD_ROOT/asan" ]; then
    run_ctest "$BUILD_ROOT/asan" || exit 3
  elif [ -d "$BUILD_ROOT/release" ]; then
    run_ctest "$BUILD_ROOT/release" || exit 3
  else
    die "No build directory found for tests."
  fi
fi

printf "[build] done\n"
exit 0
