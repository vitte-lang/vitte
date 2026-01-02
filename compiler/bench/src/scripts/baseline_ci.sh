#!/usr/bin/env sh

#
# baseline_ci.sh
#
# CI baseline generator for the Vitte benchmark harness.
#
# Responsibilities
# - Locate the bench runner executable (or use VITTE_BENCH_EXE).
# - Produce deterministic baseline reports (JSON + optional text) into
#   compiler/compat/api/reports/<platform-triple>/.
# - Emit CI outputs when running on GitHub Actions.
#
# This script is intentionally conservative about assumptions on the bench CLI.
# If your bench binary uses different flags, set VITTE_BENCH_CMD to override
# the exact command executed.
#
# Environment overrides
# - VITTE_BENCH_EXE : absolute or repo-relative path to the bench executable.
# - VITTE_BENCH_CMD : full command string to run (takes precedence).
# - VITTE_REPO_ROOT : override repository root.
# - VITTE_REPORT_DIR: override report output dir.
# - VITTE_MODE      : build mode string, default "release".
# - VITTE_TIMEOUT_S : optional timeout in seconds (requires `timeout` command).
#
# Example
#   ./compiler/bench/src/scripts/baseline_ci.sh --suite all --bins 80
#

set -eu

# best-effort pipefail (not POSIX)
( set -o pipefail ) >/dev/null 2>&1 && set -o pipefail || true

say() { printf '%s\n' "$*"; }
die() { printf 'error: %s\n' "$*" >&2; exit 1; }

usage() {
  cat <<'USAGE'
usage:
  baseline_ci.sh [options]

options:
  --out-dir <dir>     Override output directory.
  --triple <triple>   Override platform triple.
  --json <file>       Override JSON report filename (default: bench_baseline.json).
  --text <file>       Override TEXT report filename (default: bench_baseline.txt).
  --no-text           Do not emit text report.
  --suite <name>      Suite filter (passed through when using default command).
  --case <name>       Case filter (passed through when using default command).
  --bins <n>          Histogram bins (passed through when using default command).
  --iters <n>         Iterations (passed through when using default command).
  --warmup <n>        Warmup iterations (passed through when using default command).
  --timeout <sec>     Timeout in seconds (best-effort; requires `timeout`).
  -h, --help          Show this help.

environment overrides:
  VITTE_BENCH_EXE, VITTE_BENCH_CMD, VITTE_REPO_ROOT, VITTE_REPORT_DIR,
  VITTE_MODE, VITTE_TIMEOUT_S
USAGE
}

# ---------------------------------------------------------------------------
# repo root
# ---------------------------------------------------------------------------

repo_root_guess() {
  # prefer git
  if command -v git >/dev/null 2>&1; then
    r=$(git rev-parse --show-toplevel 2>/dev/null || true)
    if [ -n "$r" ]; then
      printf '%s' "$r"
      return 0
    fi
  fi

  # fall back to script location: compiler/bench/src/scripts/
  # -> go up 5 levels to reach repo root.
  d=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
  # scripts -> src -> bench -> compiler -> vitte
  # but layout can vary; use a conservative ascend.
  i=0
  while [ $i -lt 6 ]; do
    if [ -f "$d/README.md" ] || [ -f "$d/muffin.muf" ] || [ -d "$d/.git" ]; then
      printf '%s' "$d"
      return 0
    fi
    d=$(dirname -- "$d")
    i=$((i + 1))
  done

  # final: current directory
  pwd
}

REPO_ROOT=${VITTE_REPO_ROOT:-"$(repo_root_guess)"}

# ---------------------------------------------------------------------------
# platform triple (deterministic, conservative)
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

  # vendor is intentionally ‘unknown’ (bench is not toolchain-specific here)
  printf '%s-unknown-%s' "$arch" "$os"
}

# ---------------------------------------------------------------------------
# bench exe discovery
# ---------------------------------------------------------------------------

find_bench_exe() {
  # explicit wins
  if [ -n "${VITTE_BENCH_EXE:-}" ]; then
    # allow relative to repo root
    if [ -x "$VITTE_BENCH_EXE" ]; then
      printf '%s' "$VITTE_BENCH_EXE"; return 0
    fi
    if [ -x "$REPO_ROOT/$VITTE_BENCH_EXE" ]; then
      printf '%s' "$REPO_ROOT/$VITTE_BENCH_EXE"; return 0
    fi
    die "VITTE_BENCH_EXE not executable: $VITTE_BENCH_EXE"
  fi

  # common locations (keep ordered)
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
    "$REPO_ROOT/compiler/out/vitte-bench" \
    "$REPO_ROOT/compiler/out/bench" \
    ; do
    if [ -x "$p" ]; then
      printf '%s' "$p"; return 0
    fi
  done

  # fallback: PATH
  if command -v vitte-bench >/dev/null 2>&1; then
    command -v vitte-bench
    return 0
  fi
  if command -v bench >/dev/null 2>&1; then
    command -v bench
    return 0
  fi

  die "bench executable not found (set VITTE_BENCH_EXE or VITTE_BENCH_CMD)"
}

# ---------------------------------------------------------------------------
# args
# ---------------------------------------------------------------------------

TRIPLE=""
OUT_DIR=""
JSON_NAME="bench_baseline.json"
TEXT_NAME="bench_baseline.txt"
EMIT_TEXT=1
SUITE=""
CASE_=""
BINS=""
ITERS=""
WARMUP=""
TIMEOUT_S="${VITTE_TIMEOUT_S:-}" 

while [ $# -gt 0 ]; do
  case "$1" in
    --out-dir) OUT_DIR=$2; shift 2 ;;
    --triple) TRIPLE=$2; shift 2 ;;
    --json) JSON_NAME=$2; shift 2 ;;
    --text) TEXT_NAME=$2; shift 2 ;;
    --no-text) EMIT_TEXT=0; shift 1 ;;
    --suite) SUITE=$2; shift 2 ;;
    --case) CASE_=$2; shift 2 ;;
    --bins) BINS=$2; shift 2 ;;
    --iters) ITERS=$2; shift 2 ;;
    --warmup) WARMUP=$2; shift 2 ;;
    --timeout) TIMEOUT_S=$2; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) die "unknown arg: $1" ;;
  esac
done

[ -n "$TRIPLE" ] || TRIPLE="$(platform_triple_guess)"

DEFAULT_REPORT_DIR="$REPO_ROOT/compiler/compat/api/reports/$TRIPLE"
OUT_DIR=${OUT_DIR:-${VITTE_REPORT_DIR:-"$DEFAULT_REPORT_DIR"}}

# normalize to absolute for CI outputs
case "$OUT_DIR" in
  /*) : ;;
  *) OUT_DIR="$REPO_ROOT/$OUT_DIR" ;;
esac

mkdir -p "$OUT_DIR" || die "mkdir failed: $OUT_DIR"

JSON_PATH="$OUT_DIR/$JSON_NAME"
TEXT_PATH="$OUT_DIR/$TEXT_NAME"
STAMP_PATH="$OUT_DIR/bench_stamp.txt"

MODE=${VITTE_MODE:-"release"}

# ---------------------------------------------------------------------------
# stamp file (useful for CI artifacts)
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
  echo "uname_s=$(uname -s 2>/dev/null || echo unknown)"
  echo "uname_m=$(uname -m 2>/dev/null || echo unknown)"
  echo "date_utc=$(date -u '+%Y-%m-%dT%H:%M:%SZ' 2>/dev/null || echo unknown)"
} >"$STAMP_PATH" || true

# ---------------------------------------------------------------------------
# Execute bench
# ---------------------------------------------------------------------------

run_with_timeout() {
  if [ -n "$TIMEOUT_S" ] && command -v timeout >/dev/null 2>&1; then
    timeout "$TIMEOUT_S" sh -c "$1"
  else
    sh -c "$1"
  fi
}

# Override: full command
if [ -n "${VITTE_BENCH_CMD:-}" ]; then
  say "[bench] using VITTE_BENCH_CMD"
  say "[bench] out: $JSON_PATH"

  # Provide conventional variables for the command.
  export VITTE_BENCH_OUT_JSON="$JSON_PATH"
  export VITTE_BENCH_OUT_TEXT="$TEXT_PATH"
  export VITTE_BENCH_OUT_DIR="$OUT_DIR"
  export VITTE_BENCH_TRIPLE="$TRIPLE"
  export VITTE_BENCH_MODE="$MODE"

  run_with_timeout "$VITTE_BENCH_CMD" || die "bench command failed"
else
  BENCH_EXE=$(find_bench_exe)

  # Default command assumes a reasonably standard bench CLI.
  # If your CLI differs, set VITTE_BENCH_CMD.
  cmd="$BENCH_EXE"

  # Try to be compatible with common flag naming.
  # We pass both single-dash and double-dash variants only if requested via filters.
  cmd="$cmd --mode $MODE"
  cmd="$cmd --report-json \"$JSON_PATH\""
  if [ $EMIT_TEXT -ne 0 ]; then
    cmd="$cmd --report-text \"$TEXT_PATH\""
  fi
  cmd="$cmd --platform \"$TRIPLE\""

  if [ -n "$SUITE" ]; then cmd="$cmd --suite \"$SUITE\""; fi
  if [ -n "$CASE_" ]; then cmd="$cmd --case \"$CASE_\""; fi
  if [ -n "$BINS" ]; then cmd="$cmd --bins $BINS"; fi
  if [ -n "$ITERS" ]; then cmd="$cmd --iters $ITERS"; fi
  if [ -n "$WARMUP" ]; then cmd="$cmd --warmup $WARMUP"; fi

  say "[bench] exe: $BENCH_EXE"
  say "[bench] triple: $TRIPLE"
  say "[bench] out: $JSON_PATH"
  if [ $EMIT_TEXT -ne 0 ]; then say "[bench] text: $TEXT_PATH"; fi
  say "[bench] cmd: $cmd"

  run_with_timeout "$cmd" || die "bench run failed"
fi

# ---------------------------------------------------------------------------
# Post checks
# ---------------------------------------------------------------------------

[ -f "$JSON_PATH" ] || die "missing JSON report: $JSON_PATH"

if [ $EMIT_TEXT -ne 0 ]; then
  # text report is optional; do not hard-fail if CLI didn't produce it
  if [ ! -f "$TEXT_PATH" ]; then
    say "[bench] warning: text report not found: $TEXT_PATH"
  fi
fi

# ---------------------------------------------------------------------------
# GitHub Actions outputs
# ---------------------------------------------------------------------------

if [ -n "${GITHUB_OUTPUT:-}" ]; then
  {
    echo "bench_triple=$TRIPLE"
    echo "bench_report_dir=$OUT_DIR"
    echo "bench_report_json=$JSON_PATH"
    if [ $EMIT_TEXT -ne 0 ]; then
      echo "bench_report_text=$TEXT_PATH"
    fi
    echo "bench_stamp=$STAMP_PATH"
  } >>"$GITHUB_OUTPUT" || true
fi

say "[bench] done"