

#!/usr/bin/env sh
# /Users/vincent/Documents/Github/vitte/std/scripts/lint_std.sh
# -----------------------------------------------------------------------------
# std/scripts/lint_std.sh
# -----------------------------------------------------------------------------
# Lint/format driver for Vitte stdlib.
#
# MAX goals:
# - Provide one entrypoint for style checks across std/.
# - POSIX sh, strict mode.
# - Best-effort: if a tool is missing, the script reports it and either fails
#   (default) or skips (with --allow-missing).
# - Supports:
#     * shellcheck on std/scripts/*.sh
#     * editorconfig-checker (optional)
#     * clang-format (optional) on C/C headers under std/include
#     * vitte formatter (vfmt) if available (optional)
#     * whitespace and EOL sanity checks
# - CI-friendly and deterministic.
#
# Usage:
#   ./std/scripts/lint_std.sh [options]
#
# Options:
#   -C <dir>            cd before running (auto-detect repo root by default)
#   --fix               apply fixes where supported (vfmt, clang-format)
#   --allow-missing     do not fail if optional tools are missing
#   --shell             run shell lint (default: on)
#   --no-shell          disable shell lint
#   --editorconfig      run editorconfig checker (default: off)
#   --clang-format      run clang-format checks (default: off)
#   --vfmt              run vfmt formatter checks (default: off)
#   --ws                run whitespace checks (default: on)
#   -v                  verbose
#   -h                  help
# -----------------------------------------------------------------------------

set -eu

say() { printf '%s\n' "$*"; }

die() {
  printf 'error: %s\n' "$*" >&2
  exit 1
}

warn() { printf 'warn: %s\n' "$*" >&2; }

need_cmd() {
  command -v "$1" >/dev/null 2>&1 || return 1
  return 0
}

script_dir() {
  d=$(CDPATH= cd -- "$(dirname -- "$0")" 2>/dev/null && pwd) || d=""
  printf '%s' "$d"
}

find_repo_root() {
  start="$1"
  cur="$start"
  while :; do
    if [ -f "$cur/VERSION" ] && [ -d "$cur/std" ]; then
      printf '%s' "$cur"
      return 0
    fi
    if [ -f "$cur/muffin.muf" ] || [ -f "$cur/muffin.lock" ]; then
      printf '%s' "$cur"
      return 0
    fi
    parent=$(dirname -- "$cur")
    [ "$parent" = "$cur" ] && break
    cur="$parent"
  done
  return 1
}

usage() {
  cat <<'EOF'
std/scripts/lint_std.sh - lint/format driver for Vitte stdlib

Usage:
  lint_std.sh [options]

Options:
  -C <dir>            Change directory before running
  --fix               Apply fixes where supported
  --allow-missing     Do not fail if optional tools are missing
  --shell             Run shell lint (default)
  --no-shell          Disable shell lint
  --editorconfig      Run editorconfig checker
  --clang-format      Run clang-format checks
  --vfmt              Run Vitte formatter checks
  --ws                Run whitespace checks (default)
  --no-ws             Disable whitespace checks
  -v                  Verbose
  -h                  Help

Tools:
  shellcheck, editorconfig-checker, clang-format, vfmt (optional)
EOF
}

# ----------------------------------------------------------------------------
# Defaults
# ----------------------------------------------------------------------------

CHDIR=""
DO_FIX=0
ALLOW_MISSING=0
DO_SHELL=1
DO_EC=0
DO_CLANGFMT=0
DO_VFMT=0
DO_WS=1
VERBOSE=0

# ----------------------------------------------------------------------------
# Parse args
# ----------------------------------------------------------------------------

while [ $# -gt 0 ]; do
  case "$1" in
    -C)
      [ $# -ge 2 ] || die "-C expects a directory"
      CHDIR="$2"
      shift 2
      ;;
    --fix)
      DO_FIX=1
      shift
      ;;
    --allow-missing)
      ALLOW_MISSING=1
      shift
      ;;
    --shell)
      DO_SHELL=1
      shift
      ;;
    --no-shell)
      DO_SHELL=0
      shift
      ;;
    --editorconfig)
      DO_EC=1
      shift
      ;;
    --clang-format)
      DO_CLANGFMT=1
      shift
      ;;
    --vfmt)
      DO_VFMT=1
      shift
      ;;
    --ws)
      DO_WS=1
      shift
      ;;
    --no-ws)
      DO_WS=0
      shift
      ;;
    -v)
      VERBOSE=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      die "unknown option: $1"
      ;;
  esac
done

# ----------------------------------------------------------------------------
# Setup
# ----------------------------------------------------------------------------

if [ -z "$CHDIR" ]; then
  sd=$(script_dir)
  if [ -n "$sd" ] && root=$(find_repo_root "$sd" 2>/dev/null); then
    CHDIR="$root"
  else
    CHDIR="$(pwd)"
  fi
fi

cd -- "$CHDIR"

if [ "$VERBOSE" -eq 1 ]; then
  set -x
fi

# Determine std directory
if [ -d "./std" ]; then
  STD_DIR="./std"
else
  # maybe already in std
  if [ -d "./scripts" ] && [ -f "./mod.muf" ]; then
    STD_DIR="."
  else
    die "could not locate std directory from: $CHDIR"
  fi
fi

fail_missing() {
  tool="$1"
  if [ "$ALLOW_MISSING" -eq 1 ]; then
    warn "missing tool (skipping): $tool"
    return 0
  fi
  die "missing required tool: $tool"
}

# ----------------------------------------------------------------------------
# 1) Shell scripts lint
# ----------------------------------------------------------------------------

lint_shell() {
  if [ "$DO_SHELL" -eq 0 ]; then
    say "shellcheck: disabled"
    return 0
  fi

  if ! need_cmd shellcheck; then
    fail_missing shellcheck
    return 0
  fi

  say "shellcheck: std/scripts/*.sh"

  # shellcheck all sh scripts
  # Use find -print0 when available? keep POSIX: no -print0.
  # Ignore vendored/generated if needed (none by default).
  find "$STD_DIR/scripts" -type f -name '*.sh' | while IFS= read -r f; do
    [ -n "$f" ] || continue
    shellcheck -s sh "$f"
  done
}

# ----------------------------------------------------------------------------
# 2) EditorConfig checker
# ----------------------------------------------------------------------------

lint_editorconfig() {
  if [ "$DO_EC" -eq 0 ]; then
    say "editorconfig: disabled"
    return 0
  fi

  # Common command names
  if need_cmd editorconfig-checker; then
    EC_CMD=editorconfig-checker
  elif need_cmd ec; then
    EC_CMD=ec
  else
    fail_missing editorconfig-checker
    return 0
  fi

  say "editorconfig: $EC_CMD"
  "$EC_CMD" -disable-trim-trailing-whitespace=false
}

# ----------------------------------------------------------------------------
# 3) clang-format checks (C headers under std/include)
# ----------------------------------------------------------------------------

clangfmt_run() {
  f="$1"
  if [ "$DO_FIX" -eq 1 ]; then
    clang-format -i "$f"
  else
    # Diff mode: fails if reformat differs
    tmp="$f.__clangfmt__"
    cp "$f" "$tmp"
    clang-format -i "$tmp"
    if ! cmp -s "$f" "$tmp"; then
      rm -f "$tmp"
      die "clang-format mismatch: $f"
    fi
    rm -f "$tmp"
  fi
}

lint_clang_format() {
  if [ "$DO_CLANGFMT" -eq 0 ]; then
    say "clang-format: disabled"
    return 0
  fi

  if ! need_cmd clang-format; then
    fail_missing clang-format
    return 0
  fi

  if [ ! -d "$STD_DIR/include" ]; then
    warn "no std/include directory; skipping clang-format"
    return 0
  fi

  say "clang-format: std/include/**/*.[ch] (fix=$DO_FIX)"

  find "$STD_DIR/include" -type f \( -name '*.h' -o -name '*.c' \) | while IFS= read -r f; do
    [ -n "$f" ] || continue
    clangfmt_run "$f"
  done
}

# ----------------------------------------------------------------------------
# 4) vfmt (Vitte formatter) checks
# ----------------------------------------------------------------------------

lint_vfmt() {
  if [ "$DO_VFMT" -eq 0 ]; then
    say "vfmt: disabled"
    return 0
  fi

  if need_cmd vfmt; then
    VFMT=vfmt
  elif need_cmd vitte-fmt; then
    VFMT=vitte-fmt
  else
    fail_missing vfmt
    return 0
  fi

  say "vfmt: $VFMT (fix=$DO_FIX)"

  if [ "$DO_FIX" -eq 1 ]; then
    "$VFMT" fmt "$STD_DIR"
  else
    "$VFMT" check "$STD_DIR"
  fi
}

# ----------------------------------------------------------------------------
# 5) Whitespace / EOL checks
# ----------------------------------------------------------------------------

# Detect CRLF and trailing whitespace.
# POSIX tools only: grep/sed/awk.

lint_ws() {
  if [ "$DO_WS" -eq 0 ]; then
    say "whitespace: disabled"
    return 0
  fi

  say "whitespace: CRLF + trailing spaces"

  # Files to check: vitte/muf/md/sh/c/h
  find "$STD_DIR" -type f \( \
    -name '*.vitte' -o -name '*.vit' -o -name '*.vitl' -o \
    -name '*.muf' -o -name '*.md' -o -name '*.sh' -o \
    -name '*.c' -o -name '*.h' -o -name '*.txt' \
  \) | while IFS= read -r f; do
    [ -n "$f" ] || continue

    # CRLF detection
    if grep -n "\r$" "$f" >/dev/null 2>&1; then
      die "CRLF detected: $f"
    fi

    # Trailing whitespace (spaces or tabs)
    if grep -n "[\t ]$" "$f" >/dev/null 2>&1; then
      die "trailing whitespace detected: $f"
    fi
  done
}

# ----------------------------------------------------------------------------
# Run
# ----------------------------------------------------------------------------

say "lint: dir=$CHDIR fix=$DO_FIX allow_missing=$ALLOW_MISSING"

lint_shell
lint_editorconfig
lint_clang_format
lint_vfmt
lint_ws

say "lint ok"