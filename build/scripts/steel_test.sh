#!/usr/bin/env sh
set -eu
# TODO: run unit/integration tests

#!/usr/bin/env sh
#
# steel_test.sh
#
# Test runner wrapper for Steel/Vitte.
#
# Goals:
#   - Provide a stable, portable entrypoint to run:
#       * unit tests (C test binaries)
#       * integration tests (compile/run sample projects)
#       * optional fuzz smoke (non-exhaustive)
#       * optional bench smoke (sanity only)
#   - Keep POSIX sh compatibility (BSD/Solaris/Linux/macOS).
#   - Avoid assuming any particular build system internals.
#
# Assumptions (repo layout):
#   - Workspace root contains muffin.muf.
#   - Build outputs are under ./target/.
#   - Optional test binaries may be placed under:
#       target/tests/unit/*
#       target/tests/integration/*
#   - Integration projects live under:
#       tests/integration/projects/<name>/muffin.muf
#
# If your current compiler does not yet emit C test binaries, this script still
# remains useful to drive `steel test` if the CLI exposes it.
#
# Usage:
#   ./build/scripts/steel_test.sh
#   ./build/scripts/steel_test.sh --unit
#   ./build/scripts/steel_test.sh --integration
#   ./build/scripts/steel_test.sh --project hello
#   ./build/scripts/steel_test.sh --target darwin_arm64 --release
#   ./build/scripts/steel_test.sh --list
#   ./build/scripts/steel_test.sh --verbose
#   ./build/scripts/steel_test.sh --clean
#

set -eu

# ------------------------------- helpers ---------------------------------

say() { printf "%s\n" "$*"; }
warn() { printf "steel_test: %s\n" "$*" >&2; }
die() { printf "steel_test: %s\n" "$*" >&2; exit 2; }

abspath() {
  _p="$1"
  if [ -z "$_p" ]; then return 1; fi
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
  _d="$1"
  [ -n "$_d" ] || return 0
  [ -d "$_d" ] || mkdir -p "$_d"
}

rm_rf() {
  _p="$1"
  [ -n "$_p" ] || return 0
  [ ! -e "$_p" ] || rm -rf "$_p"
}

# best-effort check if `steel` supports a subcommand
steel_supports() {
  _steel="$1"
  _sub="$2"
  if "$_steel" "$_sub" --help >/dev/null 2>&1; then
    return 0
  fi
  if "$_steel" help "$_sub" >/dev/null 2>&1; then
    return 0
  fi
  return 1
}

run_exe() {
  _exe="$1"; shift
  if [ ! -x "$_exe" ]; then
    die "not executable: $_exe"
  fi
  if [ "$VERBOSE" -eq 1 ]; then
    say "+ $(qpath "$_exe") $*"
  fi
  "$_exe" "$@"
}

qpath() { printf "%s" "$1" | sed 's/ /\\ /g'; }

usage() {
  cat <<EOF
Usage: steel_test.sh [options]

Selection:
  --all                 run everything (default)
  --unit                run unit tests
  --integration         run integration tests
  --fuzz-smoke          run fuzz targets briefly (best-effort)
  --bench-smoke         run micro benches briefly (best-effort)
  --project <name>      run a single integration project (e.g. hello)

Build config:
  --debug               test in debug mode (default)
  --release             test in release mode
  --target <name>       target profile/triple (passed through)
  -j, --jobs <n>        parallel jobs (passed through)

Operations:
  --list                list discovered tests/projects
  --clean               clean test outputs under target/tests
  --verbose             verbose logging
  --quiet               minimal output
  -h, --help            show help

Environment:
  STEEL                 override steel binary path
  STEELC                override steelc binary path
EOF
}

# ------------------------------- config ----------------------------------

ROOT_DIR=$(abspath "$(dirname "$0")/../..") || exit 1
TARGET_DIR="$ROOT_DIR/target"
TEST_OUT="$TARGET_DIR/tests"

MODE=debug
TARGET_NAME=""
JOBS=""

RUN_UNIT=0
RUN_INTEGRATION=0
RUN_FUZZ=0
RUN_BENCH=0
PROJECT=""

DO_LIST=0
DO_CLEAN=0
VERBOSE=0
QUIET=0

# Default: run all
DEFAULT_ALL=1

# ----------------------------- parse args --------------------------------

while [ $# -gt 0 ]; do
  case "$1" in
    --all)
      DEFAULT_ALL=0
      RUN_UNIT=1
      RUN_INTEGRATION=1
      RUN_FUZZ=1
      RUN_BENCH=1
      shift
      ;;
    --unit) DEFAULT_ALL=0; RUN_UNIT=1; shift ;;
    --integration) DEFAULT_ALL=0; RUN_INTEGRATION=1; shift ;;
    --fuzz-smoke) DEFAULT_ALL=0; RUN_FUZZ=1; shift ;;
    --bench-smoke) DEFAULT_ALL=0; RUN_BENCH=1; shift ;;
    --project) DEFAULT_ALL=0; shift; [ $# -gt 0 ] || { usage >&2; exit 1; }; PROJECT="$1"; RUN_INTEGRATION=1; shift ;;

    --debug) MODE=debug; shift ;;
    --release) MODE=release; shift ;;
    --target) shift; [ $# -gt 0 ] || { usage >&2; exit 1; }; TARGET_NAME="$1"; shift ;;
    -j|--jobs) shift; [ $# -gt 0 ] || { usage >&2; exit 1; }; JOBS="$1"; shift ;;

    --list) DO_LIST=1; DEFAULT_ALL=0; shift ;;
    --clean) DO_CLEAN=1; shift ;;
    --verbose) VERBOSE=1; shift ;;
    --quiet) QUIET=1; shift ;;

    -h|--help) usage; exit 0 ;;
    *) usage >&2; exit 1 ;;
  esac
done

if [ "$DEFAULT_ALL" -eq 1 ]; then
  RUN_UNIT=1
  RUN_INTEGRATION=1
fi

# ----------------------------- preflight ---------------------------------

[ -f "$ROOT_DIR/muffin.muf" ] || warn "missing muffin.muf at workspace root"
mkdir_p "$TEST_OUT"

# Resolve steel binaries
STEEL=${STEEL:-}
STEELC=${STEELC:-}

if [ -z "$STEEL" ]; then
  if [ -x "$TARGET_DIR/bin/steel" ]; then
    STEEL="$TARGET_DIR/bin/steel"
  elif is_cmd steel; then
    STEEL=steel
  else
    STEEL=""
  fi
fi

if [ -z "$STEELC" ]; then
  if [ -x "$TARGET_DIR/bin/steelc" ]; then
    STEELC="$TARGET_DIR/bin/steelc"
  elif is_cmd steelc; then
    STEELC=steelc
  else
    STEELC=""
  fi
fi

# ----------------------------- discovery ---------------------------------

list_unit_binaries() {
  _d="$TARGET_DIR/tests/unit"
  [ -d "$_d" ] || return 0
  # Try common naming patterns: t_*, test_*, *_test
  for f in "$_d"/*; do
    [ -e "$f" ] || continue
    if [ -f "$f" ] && [ -x "$f" ]; then
      echo "$f"
    fi
  done
}

list_integration_projects() {
  _d="$ROOT_DIR/tests/integration/projects"
  [ -d "$_d" ] || return 0
  for p in "$_d"/*; do
    [ -d "$p" ] || continue
    if [ -f "$p/muffin.muf" ] || [ -f "$p/muffin.muf" ]; then
      echo "$(basename "$p")"
    fi
  done
}

list_fuzz_targets() {
  _d="$TARGET_DIR/fuzz"
  [ -d "$_d" ] || return 0
  for f in "$_d"/*; do
    [ -e "$f" ] || continue
    if [ -f "$f" ] && [ -x "$f" ]; then
      echo "$f"
    fi
  done
}

list_bench_bins() {
  _d="$TARGET_DIR/bench"
  [ -d "$_d" ] || return 0
  for f in "$_d"/*; do
    [ -e "$f" ] || continue
    if [ -f "$f" ] && [ -x "$f" ]; then
      echo "$f"
    fi
  done
}

# ----------------------------- listing -----------------------------------

if [ "$DO_LIST" -eq 1 ]; then
  say "unit binaries (target/tests/unit):"
  list_unit_binaries | sed 's/^/  /' || true
  say ""
  say "integration projects (tests/integration/projects):"
  list_integration_projects | sed 's/^/  /' || true
  say ""
  say "fuzz targets (target/fuzz):"
  list_fuzz_targets | sed 's/^/  /' || true
  say ""
  say "bench bins (target/bench):"
  list_bench_bins | sed 's/^/  /' || true
  exit 0
fi

# ------------------------------ clean ------------------------------------

if [ "$DO_CLEAN" -eq 1 ]; then
  if [ "$QUIET" -ne 1 ]; then
    say "steel_test: cleaning $TEST_OUT"
  fi
  rm_rf "$TEST_OUT"
  mkdir_p "$TEST_OUT"
fi

# --------------------------- build invocation ----------------------------

STEEL_BUILD_ARGS="--mode $MODE --config $ROOT_DIR/muffin.muf"
if [ -n "$TARGET_NAME" ]; then
  STEEL_BUILD_ARGS="$STEEL_BUILD_ARGS --target $TARGET_NAME"
fi
if [ -n "$JOBS" ]; then
  STEEL_BUILD_ARGS="$STEEL_BUILD_ARGS --jobs $JOBS"
fi
if [ "$VERBOSE" -eq 1 ]; then
  STEEL_BUILD_ARGS="$STEEL_BUILD_ARGS --verbose"
fi
if [ "$QUIET" -eq 1 ]; then
  STEEL_BUILD_ARGS="$STEEL_BUILD_ARGS --quiet"
fi

# If `steel test` exists, prefer it.
try_steel_test() {
  [ -n "$STEEL" ] || return 1
  steel_supports "$STEEL" test || return 1

  if [ "$QUIET" -ne 1 ]; then
    say "steel_test: steel test"
  fi

  # Best-effort mapping of flags.
  _args="$STEEL_BUILD_ARGS"
  if [ "$RUN_UNIT" -eq 1 ] && [ "$RUN_INTEGRATION" -eq 0 ]; then
    _args="$_args --unit"
  fi
  if [ "$RUN_INTEGRATION" -eq 1 ] && [ "$RUN_UNIT" -eq 0 ]; then
    _args="$_args --integration"
  fi
  if [ -n "$PROJECT" ]; then
    _args="$_args --project $PROJECT"
  fi

  # shellcheck disable=SC2086
  "$STEEL" test $_args
}

# If `steel test` is not available, fallback:
#   - run C unit tests if already built
#   - for integration: build/run sample projects via `steel build` + `steel run`

# ------------------------------- unit ------------------------------------

run_unit_tests() {
  _fail=0

  _bins=$(list_unit_binaries || true)
  if [ -z "$_bins" ]; then
    warn "no unit test binaries found under target/tests/unit"
    return 0
  fi

  if [ "$QUIET" -ne 1 ]; then
    say "steel_test: unit"
  fi

  for t in $_bins; do
    if [ "$QUIET" -ne 1 ]; then
      say "  - $(basename "$t")"
    fi
    if ! run_exe "$t"; then
      warn "unit failed: $t"
      _fail=1
    fi
  done

  return "$_fail"
}

# ---------------------------- integration --------------------------------

run_integration_project() {
  _name="$1"
  _dir="$ROOT_DIR/tests/integration/projects/$_name"

  if [ ! -d "$_dir" ]; then
    warn "missing integration project: $_name"
    return 1
  fi

  _muf="$_dir/muffin.muf"
  if [ ! -f "$_muf" ]; then
    warn "missing muffin.muf for project: $_name"
    return 1
  fi

  if [ -z "$STEEL" ]; then
    warn "steel not found; cannot run integration project"
    return 1
  fi

  # Build project
  if [ "$QUIET" -ne 1 ]; then
    say "  - build $_name"
  fi

  # shellcheck disable=SC2086
  "$STEEL" build --config "$_muf" --mode "$MODE" ${TARGET_NAME:+--target "$TARGET_NAME"} ${JOBS:+--jobs "$JOBS"} \
    ${VERBOSE:+--verbose} ${QUIET:+--quiet} || return 1

  # Run project (if steel exposes run)
  if steel_supports "$STEEL" run; then
    if [ "$QUIET" -ne 1 ]; then
      say "    run $_name"
    fi
    # shellcheck disable=SC2086
    "$STEEL" run --config "$_muf" --mode "$MODE" ${TARGET_NAME:+--target "$TARGET_NAME"} ${VERBOSE:+--verbose} ${QUIET:+--quiet} || return 1
  else
    warn "steel run not available; integration project built only: $_name"
  fi

  return 0
}

run_integration_tests() {
  _fail=0

  if [ "$QUIET" -ne 1 ]; then
    say "steel_test: integration"
  fi

  if [ -n "$PROJECT" ]; then
    run_integration_project "$PROJECT" || _fail=1
    return "$_fail"
  fi

  _projects=$(list_integration_projects || true)
  if [ -z "$_projects" ]; then
    warn "no integration projects found under tests/integration/projects"
    return 0
  fi

  for p in $_projects; do
    run_integration_project "$p" || _fail=1
  done

  return "$_fail"
}

# ----------------------------- fuzz smoke --------------------------------

run_fuzz_smoke() {
  _fail=0
  _bins=$(list_fuzz_targets || true)
  if [ -z "$_bins" ]; then
    warn "no fuzz binaries found under target/fuzz"
    return 0
  fi

  if [ "$QUIET" -ne 1 ]; then
    say "steel_test: fuzz-smoke"
  fi

  for f in $_bins; do
    if [ "$QUIET" -ne 1 ]; then
      say "  - $(basename "$f") (short run)"
    fi
    # Common libFuzzer args; ignore if the target doesn't accept.
    if ! run_exe "$f" -runs=200 -max_total_time=5 >/dev/null 2>&1; then
      # Try afl++ style/empty invocation
      if ! run_exe "$f" >/dev/null 2>&1; then
        warn "fuzz target failed (smoke): $f"
        _fail=1
      fi
    fi
  done

  return "$_fail"
}

# ---------------------------- bench smoke --------------------------------

run_bench_smoke() {
  _fail=0
  _bins=$(list_bench_bins || true)
  if [ -z "$_bins" ]; then
    warn "no bench binaries found under target/bench"
    return 0
  fi

  if [ "$QUIET" -ne 1 ]; then
    say "steel_test: bench-smoke"
  fi

  for b in $_bins; do
    if [ "$QUIET" -ne 1 ]; then
      say "  - $(basename "$b") (quick)"
    fi
    # Try a minimal run; benches accept --help at least.
    if ! run_exe "$b" --help >/dev/null 2>&1; then
      warn "bench binary not responding: $b"
      _fail=1
    fi
  done

  return "$_fail"
}

# ------------------------------- main ------------------------------------

# Prefer steel test if it exists and user didn't request special modes.
if try_steel_test; then
  exit 0
fi

# Fallback execution
FAIL=0

if [ "$RUN_UNIT" -eq 1 ]; then
  run_unit_tests || FAIL=1
fi

if [ "$RUN_INTEGRATION" -eq 1 ]; then
  run_integration_tests || FAIL=1
fi

if [ "$RUN_FUZZ" -eq 1 ]; then
  run_fuzz_smoke || FAIL=1
fi

if [ "$RUN_BENCH" -eq 1 ]; then
  run_bench_smoke || FAIL=1
fi

if [ "$FAIL" -eq 0 ]; then
  [ "$QUIET" -eq 1 ] || say "steel_test: ok"
  exit 0
fi

warn "failed"
exit 1