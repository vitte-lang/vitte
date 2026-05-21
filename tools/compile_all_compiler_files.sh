#!/usr/bin/env sh
set -eu

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
SRC_ROOT="$ROOT_DIR/src/vitte/compiler"
REPORT_DIR="$ROOT_DIR/target/reports/compiler_compile_all"
BUILD_OUT_DIR="$REPORT_DIR/build_native_out"

log() { printf '[compile-all] %s\n' "$1"; }
die() { printf '[compile-all][error] %s\n' "$1" >&2; exit 1; }

[ -x "$BIN" ] || die "missing compiler binary: $BIN"
[ -d "$SRC_ROOT" ] || die "missing source root: $SRC_ROOT"

mkdir -p "$REPORT_DIR" "$BUILD_OUT_DIR"

CHECK_OK="$REPORT_DIR/check_ok.txt"
CHECK_FAIL="$REPORT_DIR/check_fail.txt"
BUILD_OK="$REPORT_DIR/build_ok.txt"
BUILD_FAIL="$REPORT_DIR/build_fail.txt"
SKIP_BUILD="$REPORT_DIR/build_skipped_non_entrypoint.txt"
SUMMARY="$REPORT_DIR/summary.txt"

: > "$CHECK_OK"
: > "$CHECK_FAIL"
: > "$BUILD_OK"
: > "$BUILD_FAIL"
: > "$SKIP_BUILD"

is_bootstrap_entrypoint() {
  f="$1"
  case "$f" in
    */src/vitte/compiler/tests/*) return 1 ;;
  esac
  grep -q '^proc[[:space:]]\+version_text()[[:space:]]*->[[:space:]]*string[[:space:]]*{' "$f" \
    && grep -q '^proc[[:space:]]\+banner_text()[[:space:]]*->[[:space:]]*string[[:space:]]*{' "$f" \
    && grep -q '^proc[[:space:]]\+main(args:[[:space:]]*list\[string\])[[:space:]]*->[[:space:]]*int[[:space:]]*{' "$f" \
    && grep -q '^export \*$' "$f" \
    && ! grep -q '^test[[:space:]]\+"[^"]\+"' "$f"
}

count=0
check_fail=0
build_ok=0
build_fail=0
build_skip=0

for f in $(rg --files "$SRC_ROOT" -g '*.vit' | sort); do
  count=$((count + 1))
  if "$BIN" check --src "$f" >"$REPORT_DIR/check.${count}.out" 2>"$REPORT_DIR/check.${count}.err"; then
    printf '%s\n' "${f#$ROOT_DIR/}" >> "$CHECK_OK"
  else
    check_fail=$((check_fail + 1))
    printf '%s\n' "${f#$ROOT_DIR/}" >> "$CHECK_FAIL"
    continue
  fi

  if is_bootstrap_entrypoint "$f"; then
    out="$BUILD_OUT_DIR/${count}.sh"
    if "$BIN" build-native --src "$f" --out "$out" >"$REPORT_DIR/build.${count}.out" 2>"$REPORT_DIR/build.${count}.err"; then
      build_ok=$((build_ok + 1))
      printf '%s\n' "${f#$ROOT_DIR/}" >> "$BUILD_OK"
    else
      build_fail=$((build_fail + 1))
      printf '%s\n' "${f#$ROOT_DIR/}" >> "$BUILD_FAIL"
    fi
  else
    build_skip=$((build_skip + 1))
    printf '%s\n' "${f#$ROOT_DIR/}" >> "$SKIP_BUILD"
  fi

done

check_ok=$((count - check_fail))

{
  printf 'total_files=%s\n' "$count"
  printf 'check_ok=%s\n' "$check_ok"
  printf 'check_fail=%s\n' "$check_fail"
  printf 'build_native_ok=%s\n' "$build_ok"
  printf 'build_native_fail=%s\n' "$build_fail"
  printf 'build_native_skipped_non_entrypoint=%s\n' "$build_skip"
} > "$SUMMARY"

log "done report=$REPORT_DIR"
cat "$SUMMARY"

if [ "$check_fail" -gt 0 ] || [ "$build_fail" -gt 0 ]; then
  exit 1
fi
