#!/usr/bin/env sh
# C:\Users\vince\Documents\GitHub\vitte\fuzz\scripts\env_local.sh
# Local environment bootstrap for fuzzing (portable sh).
#
# This script is meant to be "sourced":
#   . ./fuzz/scripts/env_local.sh
#
# It sets sane defaults for:
#   - sanitizer + UBSan runtime opts
#   - libFuzzer / AFL convenience vars
#   - PATH additions for toolchain/bin if present
#
# You can override any variable before sourcing:
#   VITTE_FUZZ_SAN=asan . ./fuzz/scripts/env_local.sh
#
# No side effects besides exporting env vars.

# shellcheck shell=sh

# Detect if sourced (best-effort)
_is_sourced=0
# POSIX sh doesn't have BASH_SOURCE; best-effort using return
(return 0 2>/dev/null) && _is_sourced=1 || _is_sourced=0
if [ "$_is_sourced" -ne 1 ]; then
  echo "error: env_local.sh must be sourced: . ./fuzz/scripts/env_local.sh" >&2
  exit 2
fi

# Resolve repo root: fuzz/scripts -> fuzz -> repo
SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
ROOT_DIR="$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd)"
FUZZ_DIR="$ROOT_DIR/fuzz"

# User-selectable sanitizer flavor
: "${VITTE_FUZZ_SAN:=asan}"      # asan|ubsan|msan|tsan|none
: "${VITTE_FUZZ_DEBUG:=1}"       # 1 for debug symbols
: "${VITTE_FUZZ_O:=1}"           # opt level hint: 0..3
: "${VITTE_FUZZ_MAX_INPUT:=16777216}" # 16 MiB

# Convenience paths
export VITTE_ROOT="$ROOT_DIR"
export VITTE_FUZZ_DIR="$FUZZ_DIR"
export VITTE_FUZZ_OUT="${VITTE_FUZZ_OUT:-$FUZZ_DIR/out}"
export VITTE_FUZZ_FINDINGS="${VITTE_FUZZ_FINDINGS:-$FUZZ_DIR/findings}"
export VITTE_FUZZ_CORPORA="${VITTE_FUZZ_CORPORA:-$FUZZ_DIR/corpora}"
export VITTE_FUZZ_DICT="${VITTE_FUZZ_DICT:-$FUZZ_DIR/dict}"

# Toolchain PATH augmentation (if present)
if [ -d "$ROOT_DIR/toolchain/bin" ]; then
  export PATH="$ROOT_DIR/toolchain/bin:$PATH"
fi
if [ -d "$ROOT_DIR/tools/bin" ]; then
  export PATH="$ROOT_DIR/tools/bin:$PATH"
fi

# Sanitizer runtime defaults (safe for fuzzing)
# Use moderate limits; avoid symbolizer noise.
export ASAN_OPTIONS="${ASAN_OPTIONS:-\
abort_on_error=1:\
detect_leaks=1:\
handle_segv=1:\
handle_sigbus=1:\
handle_abort=1:\
allocator_may_return_null=1:\
check_initialization_order=1:\
strict_init_order=1:\
detect_stack_use_after_return=1:\
use_sigaltstack=1:\
symbolize=1:\
fast_unwind_on_malloc=0:\
malloc_context_size=20:\
exitcode=77}"

export UBSAN_OPTIONS="${UBSAN_OPTIONS:-\
halt_on_error=1:\
abort_on_error=1:\
print_stacktrace=1:\
report_error_type=1:\
silence_unsigned_overflow=0:\
exitcode=77}"

export MSAN_OPTIONS="${MSAN_OPTIONS:-\
halt_on_error=1:\
abort_on_error=1:\
print_stats=0:\
symbolize=1:\
exitcode=77}"

export TSAN_OPTIONS="${TSAN_OPTIONS:-\
halt_on_error=1:\
abort_on_error=1:\
second_deadlock_stack=1:\
history_size=7:\
symbolize=1:\
exitcode=77}"

# libFuzzer convenience vars (used by some wrappers)
export FUZZ_MAX_LEN="${FUZZ_MAX_LEN:-$VITTE_FUZZ_MAX_INPUT}"
export FUZZ_RUNS="${FUZZ_RUNS:-0}"              # 0 = infinite
export FUZZ_TIMEOUT="${FUZZ_TIMEOUT:-25}"       # seconds per input (libFuzzer -timeout)
export FUZZ_RSS_LIMIT_MB="${FUZZ_RSS_LIMIT_MB:-0}" # 0 = default

# AFL++ convenience vars
export AFL_SKIP_CPUFREQ="${AFL_SKIP_CPUFREQ:-1}"
export AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES="${AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES:-1}"
export AFL_NO_UI="${AFL_NO_UI:-1}"
export AFL_MAP_SIZE="${AFL_MAP_SIZE:-262144}"   # 256k
export AFL_EXIT_WHEN_DONE="${AFL_EXIT_WHEN_DONE:-1}"
export AFL_SHUFFLE_QUEUE="${AFL_SHUFFLE_QUEUE:-1}"
export AFL_DISABLE_TRIM="${AFL_DISABLE_TRIM:-0}"

# Default compiler/toolchain hints
: "${CC:=clang}"
: "${CXX:=clang++}"
: "${AR:=llvm-ar}"
: "${RANLIB:=llvm-ranlib}"
: "${NM:=llvm-nm}"

export CC CXX AR RANLIB NM

# Recommended flags per sanitizer
case "$VITTE_FUZZ_SAN" in
  asan)
    export VITTE_FUZZ_CFLAGS="${VITTE_FUZZ_CFLAGS:--g -O$VITTE_FUZZ_O -fno-omit-frame-pointer -fsanitize=address,undefined}"
    export VITTE_FUZZ_LDFLAGS="${VITTE_FUZZ_LDFLAGS:--fsanitize=address,undefined}"
    ;;
  ubsan)
    export VITTE_FUZZ_CFLAGS="${VITTE_FUZZ_CFLAGS:--g -O$VITTE_FUZZ_O -fno-omit-frame-pointer -fsanitize=undefined}"
    export VITTE_FUZZ_LDFLAGS="${VITTE_FUZZ_LDFLAGS:--fsanitize=undefined}"
    ;;
  msan)
    export VITTE_FUZZ_CFLAGS="${VITTE_FUZZ_CFLAGS:--g -O$VITTE_FUZZ_O -fno-omit-frame-pointer -fsanitize=memory -fsanitize-memory-track-origins=2}"
    export VITTE_FUZZ_LDFLAGS="${VITTE_FUZZ_LDFLAGS:--fsanitize=memory}"
    ;;
  tsan)
    export VITTE_FUZZ_CFLAGS="${VITTE_FUZZ_CFLAGS:--g -O$VITTE_FUZZ_O -fno-omit-frame-pointer -fsanitize=thread}"
    export VITTE_FUZZ_LDFLAGS="${VITTE_FUZZ_LDFLAGS:--fsanitize=thread}"
    ;;
  none)
    export VITTE_FUZZ_CFLAGS="${VITTE_FUZZ_CFLAGS:--g -O$VITTE_FUZZ_O}"
    export VITTE_FUZZ_LDFLAGS="${VITTE_FUZZ_LDFLAGS:-}"
    ;;
  *)
    echo "error: unknown VITTE_FUZZ_SAN=$VITTE_FUZZ_SAN (expected asan|ubsan|msan|tsan|none)" >&2
    return 2
    ;;
esac

# Optional debug toggles
if [ "${VITTE_FUZZ_DEBUG:-1}" -eq 0 ]; then
  # strip -g if user asked
  VITTE_FUZZ_CFLAGS="$(printf '%s' "$VITTE_FUZZ_CFLAGS" | sed 's/ -g / /g; s/^-g //; s/ -g$//')"
fi
export VITTE_FUZZ_CFLAGS VITTE_FUZZ_LDFLAGS

# Friendly summary
cat <<EOF
[vitte:fuzz] env loaded
  VITTE_ROOT=$VITTE_ROOT
  VITTE_FUZZ_SAN=$VITTE_FUZZ_SAN
  VITTE_FUZZ_CFLAGS=$VITTE_FUZZ_CFLAGS
  VITTE_FUZZ_LDFLAGS=$VITTE_FUZZ_LDFLAGS
  VITTE_FUZZ_OUT=$VITTE_FUZZ_OUT
  VITTE_FUZZ_FINDINGS=$VITTE_FUZZ_FINDINGS
  VITTE_FUZZ_CORPORA=$VITTE_FUZZ_CORPORA
  VITTE_FUZZ_DICT=$VITTE_FUZZ_DICT
EOF
