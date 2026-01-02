

#!/usr/bin/env sh
#
# perf_linux.sh
#
# Linux perf integration for the Vitte benchmark harness.
#
# Responsibilities
# - Run bench suites under `perf` with sensible, deterministic defaults.
# - Collect `perf stat` metrics and optional `perf record` profiles.
# - Store artifacts under compiler/compat/api/reports/<triple>/perf/.
#
# This script is Linux-specific. On BSD/Solaris/macOS, use the platform-specific
# scripts under compiler/bench/src/scripts/.
#
# Environment overrides
# - VITTE_BENCH_EXE      : path to bench runner executable.
# - VITTE_BENCH_CMD      : full bench command string (takes precedence).
# - VITTE_REPO_ROOT      : repository root.
# - VITTE_MODE           : build mode (default: release).
# - VITTE_TRIPLE         : platform triple override.
# - VITTE_REPORT_DIR     : override report base dir.
# - VITTE_PERF_BIN       : perf binary (default: perf).
# - VITTE_PERF_EVENTS    : perf stat events (comma-separated or perf syntax).
# - VITTE_PERF_REPEAT    : perf stat repeat count (default: 5).
# - VITTE_PERF_RECORD    : 1 to enable perf record.
# - VITTE_PERF_FREQ      : perf record frequency (default: 99).
# - VITTE_PERF_CALLGRAPH : perf record callgraph mode (default: dwarf).
# - VITTE_TIMEOUT_S      : timeout in seconds (requires `timeout`).
#
# Example
#   ./compiler/bench/src/scripts/perf_linux.sh --suite alloc --iters 1000
#

set -eu
( set -o pipefail ) >/dev/null 2>&1 && set -o pipefail || true

say() { printf '%s\n' "$*"; }
die() { printf 'error: %s\n' "$*" >&2; exit 1; }

usage() {
  cat <<'USAGE'
usage:
  perf_linux.sh [options]

options:
  --suite <name>       Suite filter passed to bench (default command).
  --case <name>        Case filter passed to bench (default command).
  --iters <n>          Iterations (default command).
  --warmup <n>         Warmup iterations (default command).
  --bins <n>           Histogram bins (default command).
  --out-dir <dir>      Override perf output directory.
  --record             Enable perf record (same as VITTE_PERF_RECORD=1).
  --no-record          Disable perf record.
  --events <list>      Override perf events.
  --repeat <n>         Override perf stat repeat.
  --freq <hz>          Override perf record frequency.
  --callgraph <mode>   Override perf record callgraph mode (default: dwarf).
  --timeout <sec>      Timeout in seconds (best-effort; requires `timeout`).
  -h, --help           Show help.

environment overrides:
  VITTE_BENCH_EXE, VITTE_BENCH_CMD, VITTE_REPO_ROOT, VITTE_MODE, VITTE_TRIPLE,
  VITTE_REPORT_DIR, VITTE_PERF_BIN, VITTE_PERF_EVENTS, VITTE_PERF_REPEAT,
  VITTE_PERF_RECORD, VITTE_PERF_FREQ, VITTE_PERF_CALLGRAPH, VITTE_TIMEOUT_S
USAGE
}

# ---------------------------------------------------------------------------
# repo root
# ---------------------------------------------------------------------------

repo_root_guess() {
  if command -v git >/dev/null 2>&1; then
    r=$(git rev-parse --show-toplevel 2>/dev/null || true)
    if [ -n "$r" ]; then
      printf '%s' "$r"
      return 0
    fi
  fi

  d=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
  i=0
  while [ $i -lt 6 ]; do
    if [ -f "$d/muffin.muf" ] || [ -d "$d/.git" ] || [ -f "$d/README.md" ]; then
      printf '%s' "$d"
      return 0
    fi
    d=$(dirname -- "$d")
    i=$((i + 1))
  done
  pwd
}

REPO_ROOT=${VITTE_REPO_ROOT:-"$(repo_root_guess)"}

# ---------------------------------------------------------------------------
# platform triple (Linux only here; keep consistent with baseline_ci.sh)
# ---------------------------------------------------------------------------

norm_arch() {
  case "$1" in
    x86_64|amd64) echo x86_64 ;;
    aarch64|arm64) echo aarch64 ;;
    armv7l|armv7) echo armv7 ;;
    armv6l|armv6) echo armv6 ;;
    i386|i486|i586|i686) echo x86 ;;
    riscv64) echo riscv64 ;;
    ppc64le) echo ppc64le ;;
    ppc64) echo ppc64 ;;
    *) echo "$1" ;;
  esac
}

TRIPLE=${VITTE_TRIPLE:-""}
if [ -z "$TRIPLE" ]; then
  arch=$(norm_arch "$(uname -m 2>/dev/null || echo unknown)")
  TRIPLE="$arch-unknown-linux"
fi

MODE=${VITTE_MODE:-"release"}

# ---------------------------------------------------------------------------
# bench exe discovery
# ---------------------------------------------------------------------------

find_bench_exe() {
  if [ -n "${VITTE_BENCH_EXE:-}" ]; then
    if [ -x "$VITTE_BENCH_EXE" ]; then printf '%s' "$VITTE_BENCH_EXE"; return 0; fi
    if [ -x "$REPO_ROOT/$VITTE_BENCH_EXE" ]; then printf '%s' "$REPO_ROOT/$VITTE_BENCH_EXE"; return 0; fi
    die "VITTE_BENCH_EXE not executable: $VITTE_BENCH_EXE"
  fi

  for p in \
    "$REPO_ROOT/compiler/bin/vitte-bench" \
    "$REPO_ROOT/compiler/bin/bench" \
    "$REPO_ROOT/bin/vitte-bench" \
    "$REPO_ROOT/bin/bench" \
    "$REPO_ROOT/out/vitte-bench" \
    "$REPO_ROOT/out/bench" \
    "$REPO_ROOT/build/vitte-bench" \
    "$REPO_ROOT/build/bench" \
    ; do
    if [ -x "$p" ]; then printf '%s' "$p"; return 0; fi
  done

  if command -v vitte-bench >/dev/null 2>&1; then command -v vitte-bench; return 0; fi
  if command -v bench >/dev/null 2>&1; then command -v bench; return 0; fi

  die "bench executable not found (set VITTE_BENCH_EXE or VITTE_BENCH_CMD)"
}

# ---------------------------------------------------------------------------
# args
# ---------------------------------------------------------------------------

OUT_DIR=""
SUITE=""
CASE_=""
BINS=""
ITERS=""
WARMUP=""
TIMEOUT_S="${VITTE_TIMEOUT_S:-}" 

PERF_BIN=${VITTE_PERF_BIN:-"perf"}
PERF_EVENTS=${VITTE_PERF_EVENTS:-"cycles,instructions,branches,branch-misses,cache-references,cache-misses"}
PERF_REPEAT=${VITTE_PERF_REPEAT:-"5"}
PERF_RECORD=${VITTE_PERF_RECORD:-"0"}
PERF_FREQ=${VITTE_PERF_FREQ:-"99"}
PERF_CALLGRAPH=${VITTE_PERF_CALLGRAPH:-"dwarf"}

while [ $# -gt 0 ]; do
  case "$1" in
    --out-dir) OUT_DIR=$2; shift 2 ;;
    --suite) SUITE=$2; shift 2 ;;
    --case) CASE_=$2; shift 2 ;;
    --bins) BINS=$2; shift 2 ;;
    --iters) ITERS=$2; shift 2 ;;
    --warmup) WARMUP=$2; shift 2 ;;
    --record) PERF_RECORD=1; shift 1 ;;
    --no-record) PERF_RECORD=0; shift 1 ;;
    --events) PERF_EVENTS=$2; shift 2 ;;
    --repeat) PERF_REPEAT=$2; shift 2 ;;
    --freq) PERF_FREQ=$2; shift 2 ;;
    --callgraph) PERF_CALLGRAPH=$2; shift 2 ;;
    --timeout) TIMEOUT_S=$2; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) die "unknown arg: $1" ;;
  esac
done

DEFAULT_BASE_DIR="$REPO_ROOT/compiler/compat/api/reports/$TRIPLE"
BASE_DIR=${VITTE_REPORT_DIR:-"$DEFAULT_BASE_DIR"}

case "$BASE_DIR" in
  /*) : ;;
  *) BASE_DIR="$REPO_ROOT/$BASE_DIR" ;;
esac

if [ -z "$OUT_DIR" ]; then
  OUT_DIR="$BASE_DIR/perf"
fi

case "$OUT_DIR" in
  /*) : ;;
  *) OUT_DIR="$REPO_ROOT/$OUT_DIR" ;;
esac

mkdir -p "$OUT_DIR" || die "mkdir failed: $OUT_DIR"

STAT_OUT="$OUT_DIR/perf_stat.txt"
STAT_CSV="$OUT_DIR/perf_stat.csv"
RECORD_OUT="$OUT_DIR/perf.data"
REPORT_OUT="$OUT_DIR/perf_report.txt"
CMD_OUT="$OUT_DIR/perf_cmd.txt"
STAMP_OUT="$OUT_DIR/perf_stamp.txt"

# ---------------------------------------------------------------------------
# Preflight
# ---------------------------------------------------------------------------

[ "$(uname -s 2>/dev/null || echo unknown)" = "Linux" ] || die "This script is Linux-only"

command -v "$PERF_BIN" >/dev/null 2>&1 || die "perf not found: $PERF_BIN"

# Some environments require relaxed paranoia for record/stat. We do not auto-change.
# If perf fails with "kernel.perf_event_paranoid", set it in your CI image.

BENCH_EXE=""
if [ -z "${VITTE_BENCH_CMD:-}" ]; then
  BENCH_EXE=$(find_bench_exe)
fi

GIT_SHA="unknown"
if command -v git >/dev/null 2>&1; then
  GIT_SHA=$(cd "$REPO_ROOT" && git rev-parse --short HEAD 2>/dev/null || echo unknown)
fi

{
  echo "repo_root=$REPO_ROOT"
  echo "triple=$TRIPLE"
  echo "mode=$MODE"
  echo "git_sha=$GIT_SHA"
  echo "perf_bin=$PERF_BIN"
  echo "perf_events=$PERF_EVENTS"
  echo "perf_repeat=$PERF_REPEAT"
  echo "perf_record=$PERF_RECORD"
  echo "perf_freq=$PERF_FREQ"
  echo "perf_callgraph=$PERF_CALLGRAPH"
  echo "date_utc=$(date -u '+%Y-%m-%dT%H:%M:%SZ' 2>/dev/null || echo unknown)"
} >"$STAMP_OUT" || true

# ---------------------------------------------------------------------------
# Build bench cmd
# ---------------------------------------------------------------------------

run_with_timeout() {
  if [ -n "$TIMEOUT_S" ] && command -v timeout >/dev/null 2>&1; then
    timeout "$TIMEOUT_S" sh -c "$1"
  else
    sh -c "$1"
  fi
}

bench_cmd=""

if [ -n "${VITTE_BENCH_CMD:-}" ]; then
  bench_cmd="$VITTE_BENCH_CMD"
else
  bench_cmd="$BENCH_EXE --mode $MODE"

  if [ -n "$SUITE" ]; then bench_cmd="$bench_cmd --suite \"$SUITE\""; fi
  if [ -n "$CASE_" ]; then bench_cmd="$bench_cmd --case \"$CASE_\""; fi
  if [ -n "$BINS" ]; then bench_cmd="$bench_cmd --bins $BINS"; fi
  if [ -n "$ITERS" ]; then bench_cmd="$bench_cmd --iters $ITERS"; fi
  if [ -n "$WARMUP" ]; then bench_cmd="$bench_cmd --warmup $WARMUP"; fi
fi

# Record for reproducibility
{
  echo "bench_cmd=$bench_cmd"
} >"$CMD_OUT" || true

say "[perf] triple: $TRIPLE"
say "[perf] out: $OUT_DIR"
say "[perf] cmd: $bench_cmd"

# ---------------------------------------------------------------------------
# perf stat
# ---------------------------------------------------------------------------

# perf stat: write human output and CSV for machine parsing.
# -x, : CSV delimiter
# --log-fd 1: ensure it goes to stdout for capture
# We capture stderr because perf writes stats there.

stat_cmd="$PERF_BIN stat -r $PERF_REPEAT -e $PERF_EVENTS -x, -- \"sh\" -c \"$bench_cmd\""

say "[perf] stat: $stat_cmd"

# Capture both: human + csv
# Note: perf emits CSV lines only with -x; human readable is still on stderr.
# We'll store raw stderr as perf_stat.txt and also filter CSV lines into perf_stat.csv.

# shellcheck disable=SC2090
run_with_timeout "$stat_cmd" 2>"$STAT_OUT" 1>/dev/null || die "perf stat failed"

# Extract CSV lines: heuristic (commas and event name in column 4)
# Keep all lines containing at least 3 commas.
awk -F, 'NF>=4 { print $0 }' "$STAT_OUT" >"$STAT_CSV" 2>/dev/null || true

# ---------------------------------------------------------------------------
# perf record (optional)
# ---------------------------------------------------------------------------

if [ "$PERF_RECORD" = "1" ]; then
  rec_cmd="$PERF_BIN record -F $PERF_FREQ -g --call-graph $PERF_CALLGRAPH -o \"$RECORD_OUT\" -- \"sh\" -c \"$bench_cmd\""
  say "[perf] record: $rec_cmd"

  # shellcheck disable=SC2090
  run_with_timeout "$rec_cmd" 1>/dev/null 2>/dev/null || die "perf record failed"

  rep_cmd="$PERF_BIN report --stdio -i \"$RECORD_OUT\""
  say "[perf] report: $rep_cmd"

  # shellcheck disable=SC2090
  sh -c "$rep_cmd" >"$REPORT_OUT" 2>/dev/null || true
else
  say "[perf] record disabled"
fi

say "[perf] artifacts:"
say "  $STAT_OUT"
say "  $STAT_CSV"
if [ "$PERF_RECORD" = "1" ]; then
  say "  $RECORD_OUT"
  say "  $REPORT_OUT"
fi
say "  $CMD_OUT"
say "  $STAMP_OUT"

say "[perf] done"