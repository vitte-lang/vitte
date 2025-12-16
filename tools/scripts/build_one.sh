#!/usr/bin/env sh
# tools/scripts/build_once.sh
#
# One-shot build for a single configuration.
#
# Usage:
#   sh tools/scripts/build_once.sh --mode release
#   sh tools/scripts/build_once.sh --mode debug --tests on
#   sh tools/scripts/build_once.sh --mode asan --jobs 12
#   sh tools/scripts/build_once.sh --mode relwithdebinfo --bench on
#
# Exit codes:
#   0 ok
#   2 configure/build failed
#   3 tests failed

set -eu

MODE="release"   # release|debug|asan|relwithdebinfo
TESTS="off"      # on|off
BENCH="off"      # on|off
BUILD_ROOT="build"
JOBS=""

usage() {
  cat <<EOF
Usage: sh tools/scripts/build_once.sh [options]
  --mode MODE        release|debug|asan|relwithdebinfo (default: $MODE)
  --tests on|off     build+run tests (default: $TESTS)
  --bench on|off     build benchmarks (default: $BENCH)
  --build-root DIR   build directory root (default: $BUILD_ROOT)
  -j, --jobs N       parallel jobs (passed to cmake --build)
  -h, --help
EOF
}

die(){ printf "%s\n" "$*" 1>&2; exit 2; }
has(){ command -v "$1" >/dev/null 2>&1; }
cmake_bool(){ [ "$1" = "on" ] && printf "ON" || printf "OFF"; }

while [ $# -gt 0 ]; do
  case "$1" in
    --mode) shift; [ $# -gt 0 ] || die "--mode requires a value"; MODE="$1" ;;
    --tests) shift; [ $# -gt 0 ] || die "--tests requires a value"; TESTS="$1" ;;
    --bench) shift; [ $# -gt 0 ] || die "--bench requires a value"; BENCH="$1" ;;
    --build-root) shift; [ $# -gt 0 ] || die "--build-root requires a value"; BUILD_ROOT="$1" ;;
    -j|--jobs) shift; [ $# -gt 0 ] || die "--jobs requires a value"; JOBS="$1" ;;
    -h|--help) usage; exit 0 ;;
    *) die "Unknown arg: $1 (use --help)" ;;
  esac
  shift
done

[ "$TESTS" = "on" ] || [ "$TESTS" = "off" ] || die "--tests must be on|off"
[ "$BENCH" = "on" ] || [ "$BENCH" = "off" ] || die "--bench must be on|off"

has cmake || die "cmake not found"

GEN=""
has ninja && GEN="-G Ninja"

case "$MODE" in
  release) CFG="Release"; BDIR="$BUILD_ROOT/release"; ASAN="OFF" ;;
  debug) CFG="Debug"; BDIR="$BUILD_ROOT/debug"; ASAN="OFF" ;;
  asan) CFG="Debug"; BDIR="$BUILD_ROOT/asan"; ASAN="ON" ;;
  relwithdebinfo) CFG="RelWithDebInfo"; BDIR="$BUILD_ROOT/relwithdebinfo"; ASAN="OFF" ;;
  *) die "Unknown mode: $MODE" ;;
esac

mkdir -p "$BDIR"

printf "[build_once] mode=%s dir=%s cfg=%s tests=%s bench=%s\n" "$MODE" "$BDIR" "$CFG" "$TESTS" "$BENCH"

cmake -S . -B "$BDIR" $GEN \
  -DCMAKE_BUILD_TYPE="$CFG" \
  -DVITTE_ENABLE_SANITIZERS="$ASAN" \
  -DVITTE_BUILD_TESTS="$(cmake_bool "$TESTS")" \
  -DVITTE_BUILD_BENCH="$(cmake_bool "$BENCH")"

if [ -n "$JOBS" ]; then
  cmake --build "$BDIR" --config "$CFG" -- -j "$JOBS"
else
  cmake --build "$BDIR" --config "$CFG"
fi

if [ "$TESTS" = "on" ]; then
  (cd "$BDIR" && ctest --output-on-failure)
fi

printf "[build_once] done\n"
