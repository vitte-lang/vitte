#!/usr/bin/env sh
# C:\Users\vince\Documents\GitHub\vitte\tools\scripts\coverage.sh
#
# Coverage runner for Vitte (C/C99) using LLVM (clang + llvm-profdata + llvm-cov).
#
# What it does:
#   - configures a dedicated coverage build dir
#   - builds with coverage instrumentation
#   - runs ctest
#   - merges profraw -> profdata
#   - generates:
#       dist/coverage/coverage.txt
#       dist/coverage/coverage.json
#       dist/coverage/html/index.html (html dir)
#
# Requirements:
#   - cmake, ctest
#   - clang
#   - llvm-profdata, llvm-cov
#
# Usage:
#   sh tools/scripts/coverage.sh
#   sh tools/scripts/coverage.sh --jobs 12
#   sh tools/scripts/coverage.sh --build-dir build/coverage --out dist/coverage --clean
#
# Exit codes:
#   0 ok
#   2 missing tooling / configure/build failure
#   3 tests failed
#   4 coverage generation failed

set -eu

BUILD_DIR="build/coverage"
OUT_ROOT="dist/coverage"
JOBS=""
CLEAN=0

usage() {
  cat <<EOF
Usage: sh tools/scripts/coverage.sh [options]
  --build-dir DIR   build directory (default: $BUILD_DIR)
  --out DIR         output root (default: $OUT_ROOT)
  -j, --jobs N      parallel jobs for build
  --clean           remove build dir before running
  -h, --help
EOF
}

die(){ printf "%s\n" "$*" 1>&2; exit 2; }
has(){ command -v "$1" >/dev/null 2>&1; }

while [ $# -gt 0 ]; do
  case "$1" in
    --build-dir) shift; [ $# -gt 0 ] || die "--build-dir requires a value"; BUILD_DIR="$1" ;;
    --out) shift; [ $# -gt 0 ] || die "--out requires a value"; OUT_ROOT="$1" ;;
    -j|--jobs) shift; [ $# -gt 0 ] || die "--jobs requires a value"; JOBS="$1" ;;
    --clean) CLEAN=1 ;;
    -h|--help) usage; exit 0 ;;
    *) die "Unknown arg: $1" ;;
  esac
  shift
done

for t in cmake ctest clang llvm-profdata llvm-cov; do
  has "$t" || die "Missing tool in PATH: $t"
done

if [ "$CLEAN" -eq 1 ] && [ -d "$BUILD_DIR" ]; then
  rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR" "$OUT_ROOT" "$BUILD_DIR/profraw"

# Make LLVM write per-process raw profiles.
export LLVM_PROFILE_FILE="$BUILD_DIR/profraw/%p-%m.profraw"

printf "[cov] build_dir=%s\n" "$BUILD_DIR"
printf "[cov] out_root =%s\n" "$OUT_ROOT"
printf "[cov] LLVM_PROFILE_FILE=%s\n" "$LLVM_PROFILE_FILE"

COV_CFLAGS="-O0 -g -fprofile-instr-generate -fcoverage-mapping"
COV_LDFLAGS="-fprofile-instr-generate -fcoverage-mapping"

# Configure: force clang
cmake -S . -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=clang \
  -DVITTE_BUILD_TESTS=ON \
  -DVITTE_BUILD_BENCH=OFF \
  -DVITTE_ENABLE_SANITIZERS=OFF \
  -DCMAKE_C_FLAGS="$COV_CFLAGS" \
  -DCMAKE_EXE_LINKER_FLAGS="$COV_LDFLAGS" \
  -DCMAKE_SHARED_LINKER_FLAGS="$COV_LDFLAGS"

# Build
if [ -n "$JOBS" ]; then
  cmake --build "$BUILD_DIR" --config Debug -- -j "$JOBS"
else
  cmake --build "$BUILD_DIR" --config Debug
fi

# Run tests
( cd "$BUILD_DIR" && ctest --output-on-failure ) || exit 3

# Merge profraw
PROFDATA="$OUT_ROOT/coverage.profdata"
RAW_COUNT="$(find "$BUILD_DIR/profraw" -type f -name '*.profraw' 2>/dev/null | wc -l | tr -d ' ')"
[ "$RAW_COUNT" -gt 0 ] || { printf "%s\n" "No .profraw files found. Ensure tests executed instrumented binaries." 1>&2; exit 4; }

# shellcheck disable=SC2046
llvm-profdata merge -sparse $(find "$BUILD_DIR/profraw" -type f -name '*.profraw') -o "$PROFDATA" || exit 4

# Collect executables (best-effort): vittec + any test executables in build dir.
EXECUTABLES=""

if [ -x "$BUILD_DIR/vittec" ]; then
  EXECUTABLES="$EXECUTABLES $BUILD_DIR/vittec"
fi
if [ -f "$BUILD_DIR/vittec.exe" ]; then
  EXECUTABLES="$EXECUTABLES $BUILD_DIR/vittec.exe"
fi

# Add likely test executables (avoid libs/objects/CMake internals).
# This is heuristic; adjust ignore regex to reduce noise.
while IFS= read -r f; do
  EXECUTABLES="$EXECUTABLES $f"
done <<EOF
$(find "$BUILD_DIR" -type f \
  ! -path "*/CMakeFiles/*" \
  ! -name "*.o" ! -name "*.obj" ! -name "*.a" ! -name "*.lib" \
  ! -name "*.so" ! -name "*.dylib" ! -name "*.dll" \
  ! -name "*.pdb" ! -name "*.profraw" ! -name "*.profdata" \
  ! -name "*.txt" ! -name "*.json" \
  2>/dev/null | sort)
EOF

# If we still have nothing, fail.
# Trim
EXECUTABLES="$(printf "%s" "$EXECUTABLES" | awk '{$1=$1;print}')"
[ -n "$EXECUTABLES" ] || { printf "%s\n" "No executables found in build dir for llvm-cov." 1>&2; exit 4; }

TXT="$OUT_ROOT/coverage.txt"
JSON="$OUT_ROOT/coverage.json"
HTML_DIR="$OUT_ROOT/html"

rm -rf "$HTML_DIR"
mkdir -p "$HTML_DIR"

IGNORE_REGEX="(.*/_deps/.*|.*/CMakeFiles/.*|.*/tests/.*\.c)"

# HTML
# shellcheck disable=SC2086
llvm-cov show $EXECUTABLES \
  --instr-profile "$PROFDATA" \
  --format html \
  --output-dir "$HTML_DIR" \
  --ignore-filename-regex "$IGNORE_REGEX" \
  --show-line-counts-or-regions \
  --show-instantiations \
  >/dev/null || exit 4

# Text report
# shellcheck disable=SC2086
llvm-cov report $EXECUTABLES \
  --instr-profile "$PROFDATA" \
  --ignore-filename-regex "$IGNORE_REGEX" \
  > "$TXT" || exit 4

# JSON export
# shellcheck disable=SC2086
llvm-cov export $EXECUTABLES \
  --instr-profile "$PROFDATA" \
  --ignore-filename-regex "$IGNORE_REGEX" \
  > "$JSON" || exit 4

printf "[cov] outputs:\n"
printf "  %s\n" "$TXT"
printf "  %s\n" "$JSON"
printf "  %s\n" "$HTML_DIR"
exit 0
