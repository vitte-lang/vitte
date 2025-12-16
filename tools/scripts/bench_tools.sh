#!/usr/bin/env sh
# C:\Users\vince\Documents\GitHub\vitte\tools\scripts\bench_tools.sh
#
# Bench harness for Vitte tools (CMake builds, smoke runs, timing).
# Portable: macOS/Linux/WSL/MSYS2 (best-effort). Uses /usr/bin/time when available.
#
# Usage:
#   sh tools/scripts/bench_tools.sh
#   sh tools/scripts/bench_tools.sh --iters 10 --mode release --cmd "--help"
#
# Exit codes:
#   0 ok
#   1 usage / bad args
#   2 build failed
#   3 run failed

set -eu

# ----------------------------- config defaults --------------------------------
ITERS=5
MODE="release"         # release|debug|asan
CMD_ARGS="--help"      # args passed to vittec
BUILD_ROOT="build"
BIN_NAME="vittec"

# ----------------------------- helpers ----------------------------------------
die() { printf "%s\n" "$*" 1>&2; exit 1; }

has() { command -v "$1" >/dev/null 2>&1; }

now_ms() {
  # best effort wall clock in ms
  if has python3; then
    python3 - <<'PY'
import time
print(int(time.time()*1000))
PY
  else
    # seconds -> ms (low precision)
    printf "%s000\n" "$(date +%s)"
  fi
}

time_cmd_prefix() {
  # Prefer GNU time if present, fallback to shell timing.
  # We print a single line like: "real_ms=1234"
  if has /usr/bin/time; then
    # macOS /usr/bin/time supports -p but not --format; GNU supports -f.
    # We'll detect GNU by probing "-f".
    if /usr/bin/time -f "%e" sh -c ":" >/dev/null 2>&1; then
      # GNU time
      printf "/usr/bin/time -f real_s=%%e"
      return 0
    fi
    # POSIX-ish
    printf "/usr/bin/time -p"
    return 0
  fi
  printf ""
}

build_dir_for_mode() {
  case "$1" in
    release) printf "%s/release\n" "$BUILD_ROOT" ;;
    debug)   printf "%s/debug\n"   "$BUILD_ROOT" ;;
    asan)    printf "%s/asan\n"    "$BUILD_ROOT" ;;
    *) die "Unknown mode: $1 (expected release|debug|asan)" ;;
  esac
}

cmake_configure_build() {
  _mode="$1"
  _bdir="$(build_dir_for_mode "$_mode")"
  mkdir -p "$_bdir"

  case "$_mode" in
    release)
      cmake -S . -B "$_bdir" -DCMAKE_BUILD_TYPE=Release -DVITTE_BUILD_TESTS=OFF -DVITTE_BUILD_BENCH=OFF
      cmake --build "$_bdir" --config Release
      ;;
    debug)
      cmake -S . -B "$_bdir" -DCMAKE_BUILD_TYPE=Debug -DVITTE_BUILD_TESTS=OFF -DVITTE_BUILD_BENCH=OFF
      cmake --build "$_bdir" --config Debug
      ;;
    asan)
      cmake -S . -B "$_bdir" -DCMAKE_BUILD_TYPE=Debug -DVITTE_BUILD_TESTS=OFF -DVITTE_BUILD_BENCH=OFF -DVITTE_ENABLE_SANITIZERS=ON
      cmake --build "$_bdir" --config Debug
      ;;
  esac
}

resolve_bin() {
  _mode="$1"
  _bdir="$(build_dir_for_mode "$_mode")"
  if [ -x "$_bdir/$BIN_NAME" ]; then
    printf "%s/%s\n" "$_bdir" "$BIN_NAME"
    return 0
  fi
  if [ -x "$_bdir/$BIN_NAME.exe" ]; then
    printf "%s/%s.exe\n" "$_bdir" "$BIN_NAME"
    return 0
  fi
  die "Binary not found in $_bdir ($BIN_NAME or $BIN_NAME.exe)."
}

usage() {
  cat <<EOF
Usage: sh tools/scripts/bench_tools.sh [options]

Options:
  --iters N         number of iterations (default: $ITERS)
  --mode MODE       release|debug|asan (default: $MODE)
  --cmd "ARGS"      args passed to vittec (default: "$CMD_ARGS")
  --bin NAME        binary name (default: $BIN_NAME)
  --build-root DIR  build root dir (default: $BUILD_ROOT)
  -h, --help        show help

Examples:
  sh tools/scripts/bench_tools.sh --iters 10 --mode release --cmd "--help"
  sh tools/scripts/bench_tools.sh --mode debug --cmd "compile examples/hello.vitte"
EOF
}

# ----------------------------- args -------------------------------------------
while [ $# -gt 0 ]; do
  case "$1" in
    --iters) shift; [ $# -gt 0 ] || die "--iters requires a value"; ITERS="$1" ;;
    --mode) shift; [ $# -gt 0 ] || die "--mode requires a value"; MODE="$1" ;;
    --cmd) shift; [ $# -gt 0 ] || die "--cmd requires a value"; CMD_ARGS="$1" ;;
    --bin) shift; [ $# -gt 0 ] || die "--bin requires a value"; BIN_NAME="$1" ;;
    --build-root) shift; [ $# -gt 0 ] || die "--build-root requires a value"; BUILD_ROOT="$1" ;;
    -h|--help) usage; exit 0 ;;
    *) die "Unknown arg: $1 (use --help)" ;;
  esac
  shift
done

# ----------------------------- run --------------------------------------------
printf "[bench] mode=%s iters=%s cmd=%s\n" "$MODE" "$ITERS" "$CMD_ARGS"

if ! has cmake; then
  die "cmake not found in PATH."
fi

printf "[bench] configuring + building...\n"
if ! cmake_configure_build "$MODE"; then
  exit 2
fi

BIN_PATH="$(resolve_bin "$MODE")"
printf "[bench] binary=%s\n" "$BIN_PATH"

# warmup
printf "[bench] warmup...\n"
if ! "$BIN_PATH" $CMD_ARGS >/dev/null 2>&1; then
  printf "[bench] warmup failed (non-zero exit)\n" 1>&2
  exit 3
fi

printf "[bench] running %s iterations...\n" "$ITERS"

# Collect wall times (ms) into a temp file.
TMP="$(mktemp 2>/dev/null || printf "%s\n" "$BUILD_ROOT/.bench_tools_tmp.$$")"
: > "$TMP"

i=1
while [ "$i" -le "$ITERS" ]; do
  start="$(now_ms)"
  if ! "$BIN_PATH" $CMD_ARGS >/dev/null 2>&1; then
    printf "[bench] iteration %s failed\n" "$i" 1>&2
    rm -f "$TMP" || true
    exit 3
  fi
  end="$(now_ms)"
  dur=$((end - start))
  printf "%s\n" "$dur" >> "$TMP"
  printf "  iter=%s dur_ms=%s\n" "$i" "$dur"
  i=$((i + 1))
done

# Compute stats (min/mean/median/p95/max) with python if possible, else fallback.
printf "[bench] stats:\n"
if has python3; then
  python3 - "$TMP" <<'PY'
import sys, statistics, math
path = sys.argv[1]
xs = [int(line.strip()) for line in open(path, "r", encoding="utf-8") if line.strip()]
xs_sorted = sorted(xs)
n = len(xs_sorted)
def pct(p):
  if n == 0:
    return 0
  k = (n - 1) * (p / 100.0)
  f = math.floor(k); c = math.ceil(k)
  if f == c:
    return xs_sorted[int(k)]
  return int(xs_sorted[f] + (xs_sorted[c] - xs_sorted[f]) * (k - f))
print(f"  n={n}")
print(f"  min_ms={xs_sorted[0]}")
print(f"  mean_ms={int(statistics.mean(xs_sorted))}")
print(f"  median_ms={int(statistics.median(xs_sorted))}")
print(f"  p95_ms={pct(95)}")
print(f"  max_ms={xs_sorted[-1]}")
PY
else
  # crude fallback: min/max + average integer
  min=999999999
  max=0
  sum=0
  n=0
  while IFS= read -r v; do
    [ -n "$v" ] || continue
    n=$((n + 1))
    sum=$((sum + v))
    [ "$v" -lt "$min" ] && min="$v"
    [ "$v" -gt "$max" ] && max="$v"
  done < "$TMP"
  avg=$((sum / (n > 0 ? n : 1)))
  printf "  n=%s\n" "$n"
  printf "  min_ms=%s\n" "$min"
  printf "  mean_ms=%s\n" "$avg"
  printf "  max_ms=%s\n" "$max"
fi

rm -f "$TMP" || true
printf "[bench] done\n"
