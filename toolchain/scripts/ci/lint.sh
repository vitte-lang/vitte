#!/usr/bin/env bash
# ============================================================
# vitte — CI linter
# Location: toolchain/scripts/ci/lint.sh
# ============================================================

set -euo pipefail

# ----------------------------
# Configuration
# ----------------------------
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"
MODE="${MODE:-ci}"             # ci | local
JOBS="${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)}"

# Tools (optional)
SHELLCHECK="${SHELLCHECK:-shellcheck}"
CLANG_TIDY="${CLANG_TIDY:-clang-tidy}"
CPPCHECK="${CPPCHECK:-cppcheck}"

# ----------------------------
# Helpers
# ----------------------------
log() { printf "[lint] %s\n" "$*"; }
warn() { printf "[lint][warn] %s\n" "$*" >&2; }
die() { printf "[lint][error] %s\n" "$*" >&2; exit 1; }

has() { command -v "$1" >/dev/null 2>&1; }

cd "$ROOT_DIR"

log "mode=$MODE root=$ROOT_DIR"

# ----------------------------
# Shell scripts
# ----------------------------
log "shellcheck"
if has "$SHELLCHECK"; then
  SH_FILES=$(git ls-files "*.sh" || true)
  if [ -n "$SH_FILES" ]; then
    echo "$SH_FILES" | xargs -r -n 20 -P "$JOBS" \
      "$SHELLCHECK" -x || die "shellcheck failed"
  fi
else
  warn "shellcheck not found — skipping"
fi

# ----------------------------
# Vitte / Vit / Grammar
# ----------------------------
log "vitte static sanity"
VITTE_FILES=$(git ls-files "*.vit" "*.vitte" || true)
if [ -n "$VITTE_FILES" ]; then
  # 1) UTF-8 validity
  echo "$VITTE_FILES" | xargs -r -n 50 -P "$JOBS" \
    iconv -f utf-8 -t utf-8 >/dev/null 2>&1 \
    || die "non-utf8 encoding detected in vitte sources"

  # 2) Forbidden tabs (indentation policy)
  if grep -RIn $'\t' -- "$VITTE_FILES" >/dev/null; then
    die "tab character found in vitte sources (use spaces)"
  fi

  # 3) Block terminator policy (.end)
  # Heuristic: any line starting with '{' or '}' is forbidden
  if grep -RIn '^[[:space:]]*[{}]' -- "$VITTE_FILES" >/dev/null; then
    die "brace detected in vitte sources (blocks must end with .end)"
  fi
fi

# ----------------------------
# C / C++ (optional)
# ----------------------------
log "c/c++ lint"
CC_FILES=$(git ls-files "*.c" "*.h" "*.cpp" "*.hpp" || true)

if [ -n "$CC_FILES" ]; then
  if has "$CLANG_TIDY" && [ -f "compile_commands.json" ]; then
    log "clang-tidy (compile_commands.json detected)"
    echo "$CC_FILES" | xargs -r -n 5 -P "$JOBS" \
      "$CLANG_TIDY" || die "clang-tidy failed"
  else
    warn "clang-tidy skipped (missing tool or compile_commands.json)"
  fi

  if has "$CPPCHECK"; then
    log "cppcheck"
    "$CPPCHECK" \
      --enable=warning,style,performance \
      --std=c++20 \
      --quiet \
      $(echo "$CC_FILES") || die "cppcheck failed"
  else
    warn "cppcheck not found — skipping"
  fi
fi

# ----------------------------
# Git hygiene
# ----------------------------
log "git hygiene"

# 1) No unresolved conflict markers
if git grep -n '<<<<<<<\|=======\|>>>>>>>' >/dev/null; then
  die "git conflict markers detected"
fi

# 2) No executable bits on non-scripts
BAD_EXEC=$(git ls-files --stage | awk '$1 ~ /^100755/ && $4 !~ /\.sh$/ {print $4}')
if [ -n "$BAD_EXEC" ]; then
  die "unexpected executable files: $BAD_EXEC"
fi

# ----------------------------
# Final
# ----------------------------
log "lint complete"