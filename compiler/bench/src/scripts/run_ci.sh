#!/usr/bin/env sh
#
# run_ci.sh
#
# CI runner wrapper for the Vitte benchmark harness.
#
# Responsibilities
# - Locate the bench runner executable (or use VITTE_BENCH_EXE).
# - Execute a CI-friendly benchmark run with deterministic defaults.
# - Write artifacts under compiler/compat/api/reports/<triple>/ci/.
# - Produce a stamp file for reproducibility and emit GitHub Actions outputs.
#
# This script is meant to be a "simple" entrypoint compared to baseline_ci.sh:
# - baseline_ci.sh is the baseline generator used for compat/regression.
# - run_ci.sh is a lightweight CI execution wrapper to attach artifacts to a run.
#
# Environment overrides
# - VITTE_BENCH_EXE      : path to bench executable.
# - VITTE_BENCH_CMD      : full command string to execute (takes precedence).
# - VITTE_REPO_ROOT      : repository root override.
# - VITTE_REPORT_DIR     : override report base dir.
# - VITTE_TRIPLE         : platform triple override.
# - VITTE_MODE           : build mode string (default: release).
# - VITTE_ITERS          : iterations (default: 50).
# - VITTE_WARMUP         : warmup iterations (default: 5).
# - VITTE_BINS           : histogram bins (optional).
# - VITTE_SUITE          : suite filter (optional).
# - VITTE_CASE           : case filter (optional).
# - VITTE_TIMEOUT_S      : timeout in seconds (requires `timeout`).
# - VITTE_JSON_NAME      : json output filename (default: bench_ci.json).
# - VITTE_TEXT_NAME      : text output filename (default: bench_ci.txt).
# - VITTE_NO_TEXT        : set to 1 to disable text report.
#
# CLI options
# - --suite/--case/--iters/--warmup/--bins mirror env overrides.
# - --out-dir overrides output directory.
# - --timeout overrides VITTE_TIMEOUT_S.
#

set -eu
( set -o pipefail ) >/dev/null 2>&1 && set -o pipefail || true

say() { printf '%s\n' "$*"; }
die() { printf 'error: %s\n' "$*" >&2; exit 1; }

usage() {
  cat <<'USAGE'
usage:
  run_ci.sh [options]

options:
  --out-dir <dir>   Override output directory.
  --triple <triple> Override platform triple.
  --suite <name>    Suite filter.
  --case <name>     Case filter.
  --iters <n>       Iterations.
  --warmup <n>      Warmup iterations.
  --bins <n>        Histogram bins.
  --timeout <sec>   Timeout seconds (best-effort; requires `timeout`).
  --no-text         Disable text report.
  -h, --help        Show this help.

environment overrides:
  VITTE_BENCH_EXE, VITTE_BENCH_CMD, VITTE_REPO_ROOT, VITTE_REPORT_DIR,
  VITTE_TRIPLE, VITTE_MODE, VITTE_ITERS, VITTE_WARMUP, VITTE_BINS,
  VITTE_SUITE, VITTE_CASE, VITTE_TIMEOUT_S, VITTE_JSON_NAME,
  VITTE_TEXT_NAME, VITTE_NO_TEXT
USAGE
}

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

norm_os() {
  case "$1" in
    Darwin) echo darwin ;;
    Linux) echo linux ;;
    FreeBSD) echo freebsd ;;
    NetBSD) echo netbsd ;;
    OpenBSD) echo openbsd ;;
    DragonFly) echo dragonflybsd ;;
    SunOS) echo solaris ;;
    *) echo "$(printf '%s' "$1" | tr '[:upper:]' '[:lower:]')" ;;
  esac
}

platform_triple_guess() {
  os=$(uname -s 2>/dev/null || echo unknown)
  arch=$(uname -m 2>/dev/null || echo unknown)
  os=$(norm_os "$os")
  arch=$(norm_arch "$arch")
  printf '%s-unknown-%s' "$arch" "$os"
}

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
    "$REPO_ROOT/compiler/bench/bin/vitte-bench" \
    "$REPO_ROOT/compiler/bench/bin/bench" \
    ; do
    if [ -x "$p" ]; then printf '%s' "$p"; return 0; fi
  done

  if command -v vitte-bench >/dev/null 2>&1; then command -v vitte-bench; return 0; fi
  if command -v bench >/dev/null 2>&1; then command -v bench; return 0; fi

  die "bench executable not found (set VITTE_BENCH_EXE or VITTE_BENCH_CMD)"
}

run_with_timeout() {
  if [ -n "$TIMEOUT_S" ] && command -v timeout >/dev/null 2>&1; then
    timeout "$TIMEOUT_S" sh -c "$1"
  else
    sh -c "$1"
  fi
}

# ---------------------------------------------------------------------------
# Parse args
# ---------------------------------------------------------------------------

REPO_ROOT=${VITTE_REPO_ROOT:-"$(repo_root_guess)"}

OUT_DIR=""
TRIPLE="${VITTE_TRIPLE:-""}"
SUITE="${VITTE_SUITE:-""}"
CASE_="${VITTE_CASE:-""}"
ITERS="${VITTE_ITERS:-"50"}"
WARMUP="${VITTE_WARMUP:-"5"}"
BINS="${VITTE_BINS:-""}"
TIMEOUT_S="${VITTE_TIMEOUT_S:-""}"
MODE="${VITTE_MODE:-"release"}"

JSON_NAME="${VITTE_JSON_NAME:-"bench_ci.json"}"
TEXT_NAME="${VITTE_TEXT_NAME:-"bench_ci.txt"}"
EMIT_TEXT=1
if [ "${VITTE_NO_TEXT:-0}" = "1" ]; then
  EMIT_TEXT=0
fi

while [ $# -gt 0 ]; do
  case "$1" in
    --out-dir) OUT_DIR=$2; shift 2 ;;
    --triple) TRIPLE=$2; shift 2 ;;
    --suite) SUITE=$2; shift 2 ;;
    --case) CASE_=$2; shift 2 ;;
    --iters) ITERS=$2; shift 2 ;;
    --warmup) WARMUP=$2; shift 2 ;;
    --bins) BINS=$2; shift 2 ;;
    --timeout) TIMEOUT_S=$2; shift 2 ;;
    --no-text) EMIT_TEXT=0; shift 1 ;;
    -h|--help) usage; exit 0 ;;
    *) die "unknown arg: $1" ;;
  esac
done

[ -n "$TRIPLE" ] || TRIPLE="$(platform_triple_guess)"

DEFAULT_BASE_DIR="$REPO_ROOT/compiler/compat/api/reports/$TRIPLE"
BASE_DIR=${VITTE_REPORT_DIR:-"$DEFAULT_BASE_DIR"}

case "$BASE_DIR" in
  /*) : ;;
  *) BASE_DIR="$REPO_ROOT/$BASE_DIR" ;;
esac

if [ -z "$OUT_DIR" ]; then
  OUT_DIR="$BASE_DIR/ci"
fi

case "$OUT_DIR" in
  /*) : ;;
  *) OUT_DIR="$REPO_ROOT/$OUT_DIR" ;;
esac

mkdir -p "$OUT_DIR" || die "mkdir failed: $OUT_DIR"

JSON_PATH="$OUT_DIR/$JSON_NAME"
TEXT_PATH="$OUT_DIR/$TEXT_NAME"
STAMP_PATH="$OUT_DIR/bench_ci_stamp.txt"
CMD_PATH="$OUT_DIR/bench_ci_cmd.txt"

# ---------------------------------------------------------------------------
# Stamp
# ---------------------------------------------------------------------------

GIT_SHA="unknown"
if command -v git >/dev/null 2>&1; then
  GIT_SHA=$(cd "$REPO_ROOT" && git rev-parse --short HEAD 2>/dev/null || echo unknown)
fi

{
  echo "repo_root=$REPO_ROOT"
  echo "triple=$TRIPLE"
  echo "mode=$MODE"
  echo "git_sha=$GIT_SHA"
  echo "iters=$ITERS"
  echo "warmup=$WARMUP"
  echo "bins=$BINS"
  echo "suite=$SUITE"
  echo "case=$CASE_"
  echo "uname_s=$(uname -s 2>/dev/null || echo unknown)"
  echo "uname_m=$(uname -m 2>/dev/null || echo unknown)"
  echo "date_utc=$(date -u '+%Y-%m-%dT%H:%M:%SZ' 2>/dev/null || echo unknown)"
} >"$STAMP_PATH" || true

# ---------------------------------------------------------------------------
# Execute
# ---------------------------------------------------------------------------

bench_cmd=""

if [ -n "${VITTE_BENCH_CMD:-}" ]; then
  bench_cmd="$VITTE_BENCH_CMD"
else
  BENCH_EXE=$(find_bench_exe)

  # Default assumes a standard bench CLI. If your CLI differs, set VITTE_BENCH_CMD.
  bench_cmd="$BENCH_EXE --mode $MODE"
  bench_cmd="$bench_cmd --platform \"$TRIPLE\""

  # Reports
  bench_cmd="$bench_cmd --report-json \"$JSON_PATH\""
  if [ $EMIT_TEXT -ne 0 ]; then
    bench_cmd="$bench_cmd --report-text \"$TEXT_PATH\""
  fi

  # CI-friendly run parameters
  bench_cmd="$bench_cmd --iters $ITERS --warmup $WARMUP"

  if [ -n "$BINS" ]; then bench_cmd="$bench_cmd --bins $BINS"; fi
  if [ -n "$SUITE" ]; then bench_cmd="$bench_cmd --suite \"$SUITE\""; fi
  if [ -n "$CASE_" ]; then bench_cmd="$bench_cmd --case \"$CASE_\""; fi
fi

{
  echo "bench_cmd=$bench_cmd"
} >"$CMD_PATH" || true

say "[ci] triple: $TRIPLE"
say "[ci] out: $OUT_DIR"
say "[ci] json: $JSON_PATH"
if [ $EMIT_TEXT -ne 0 ]; then say "[ci] text: $TEXT_PATH"; fi
say "[ci] cmd: $bench_cmd"

run_with_timeout "$bench_cmd" || die "bench failed"

[ -f "$JSON_PATH" ] || die "missing JSON report: $JSON_PATH"
if [ $EMIT_TEXT -ne 0 ] && [ ! -f "$TEXT_PATH" ]; then
  say "[ci] warning: text report not found: $TEXT_PATH"
fi

# ---------------------------------------------------------------------------
# GitHub Actions outputs
# ---------------------------------------------------------------------------

if [ -n "${GITHUB_OUTPUT:-}" ]; then
  {
    echo "bench_triple=$TRIPLE"
    echo "bench_ci_dir=$OUT_DIR"
    echo "bench_ci_json=$JSON_PATH"
    if [ $EMIT_TEXT -ne 0 ]; then
      echo "bench_ci_text=$TEXT_PATH"
    fi
    echo "bench_ci_stamp=$STAMP_PATH"
    echo "bench_ci_cmd=$CMD_PATH"
  } >>"$GITHUB_OUTPUT" || true
fi

say "[ci] done"
