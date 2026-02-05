#!/usr/bin/env bash
# ============================================================
# vitte — CI formatter
# Location: toolchain/scripts/ci/format.sh
# ============================================================

set -euo pipefail

# ----------------------------
# Configuration
# ----------------------------
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"
MODE="${MODE:-check}"          # check | fix
JOBS="${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)}"

# Tools (override if needed)
SHFMT="${SHFMT:-shfmt}"
PRETTIER="${PRETTIER:-prettier}"
CLANG_FORMAT="${CLANG_FORMAT:-clang-format}"

# ----------------------------
# Helpers
# ----------------------------
log() { printf "[format] %s\n" "$*"; }
die() { printf "[format][error] %s\n" "$*" >&2; exit 1; }

need() {
  command -v "$1" >/dev/null 2>&1 || die "missing tool: $1"
}

run_check() {
  log "mode=check"
}

run_fix() {
  log "mode=fix"
}

# ----------------------------
# Pre-flight
# ----------------------------
need "$SHFMT"
# prettier / clang-format are optional; detect later

cd "$ROOT_DIR"

case "$MODE" in
  check) run_check ;;
  fix)   run_fix ;;
  *)     die "MODE must be 'check' or 'fix'" ;;
esac

# ----------------------------
# Shell scripts
# ----------------------------
log "shell formatting"
SHELL_FILES=$(git ls-files "*.sh" || true)
if [ -n "$SHELL_FILES" ]; then
  if [ "$MODE" = "check" ]; then
    echo "$SHELL_FILES" | xargs -r -n 50 -P "$JOBS" \
      "$SHFMT" -d -i 2 -ci || die "shell format check failed"
  else
    echo "$SHELL_FILES" | xargs -r -n 50 -P "$JOBS" \
      "$SHFMT" -w -i 2 -ci
  fi
fi

# ----------------------------
# Vitte / Vit / Vitte grammar (style pass-through)
# (No auto-formatter enforced; whitespace sanity only)
# ----------------------------
log "vitte sources sanity"
VITTE_FILES=$(git ls-files "*.vit" "*.vitte" || true)
if [ -n "$VITTE_FILES" ]; then
  # enforce LF + no trailing spaces
  if [ "$MODE" = "check" ]; then
    echo "$VITTE_FILES" | xargs -r -n 50 -P "$JOBS" \
      awk '{
        if (sub(/[ \t]+$/, "", $0)) exit 1
      }' || die "trailing whitespace in vitte sources"
  else
    echo "$VITTE_FILES" | xargs -r -n 50 -P "$JOBS" \
      sed -i.bak -E 's/[ \t]+$//' && find . -name "*.bak" -delete
  fi
fi

# ----------------------------
# JSON / YAML / Markdown (optional)
# ----------------------------
if command -v "$PRETTIER" >/dev/null 2>&1; then
  log "prettier formatting"
  PRETTIER_FILES=$(git ls-files "*.json" "*.yml" "*.yaml" "*.md" || true)
  if [ -n "$PRETTIER_FILES" ]; then
    if [ "$MODE" = "check" ]; then
      echo "$PRETTIER_FILES" | xargs -r -n 20 -P "$JOBS" \
        "$PRETTIER" --check || die "prettier check failed"
    else
      echo "$PRETTIER_FILES" | xargs -r -n 20 -P "$JOBS" \
        "$PRETTIER" --write
    fi
  fi
else
  log "prettier not found — skipping"
fi

# ----------------------------
# C / C++ (optional)
# ----------------------------
if command -v "$CLANG_FORMAT" >/dev/null 2>&1 && [ -f ".clang-format" ]; then
  log "clang-format"
  CC_FILES=$(git ls-files "*.c" "*.h" "*.cpp" "*.hpp" || true)
  if [ -n "$CC_FILES" ]; then
    if [ "$MODE" = "check" ]; then
      echo "$CC_FILES" | xargs -r -n 50 -P "$JOBS" \
        "$CLANG_FORMAT" --dry-run --Werror || die "clang-format check failed"
    else
      echo "$CC_FILES" | xargs -r -n 50 -P "$JOBS" \
        "$CLANG_FORMAT" -i
    fi
  fi
else
  log "clang-format not configured — skipping"
fi

# ----------------------------
# Final
# ----------------------------
log "formatting complete"