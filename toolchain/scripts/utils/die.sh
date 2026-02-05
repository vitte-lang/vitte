#!/usr/bin/env bash
# ============================================================
# vitte — die utility
# Location: toolchain/scripts/utils/die.sh
# ============================================================

# Usage:
#   source toolchain/scripts/utils/die.sh
#   die "message" [exit_code]
#   die_with_ctx "message" [exit_code]

set -euo pipefail

# ----------------------------
# Configuration
# ----------------------------
DIE_PREFIX="${DIE_PREFIX:-[vitte][error]}"
DIE_SHOW_CTX="${DIE_SHOW_CTX:-0}"   # 1 to show call context
DIE_EXIT_CODE="${DIE_EXIT_CODE:-1}"

# ----------------------------
# Helpers
# ----------------------------
_print_ctx() {
  # Print a compact call context (best-effort)
  # shellcheck disable=SC2154
  if [ "${#BASH_SOURCE[@]}" -gt 1 ]; then
    local i
    for ((i=1; i<${#BASH_SOURCE[@]}; i++)); do
      printf "  at %s:%s (%s)\n" \
        "${BASH_SOURCE[$i]}" \
        "${BASH_LINENO[$((i-1))]}" \
        "${FUNCNAME[$i]}"
    done
  fi
}

die() {
  local msg="${1:-unknown error}"
  local code="${2:-$DIE_EXIT_CODE}"
  printf "%s %s\n" "$DIE_PREFIX" "$msg" >&2
  exit "$code"
}

die_with_ctx() {
  local msg="${1:-unknown error}"
  local code="${2:-$DIE_EXIT_CODE}"
  printf "%s %s\n" "$DIE_PREFIX" "$msg" >&2
  if [ "$DIE_SHOW_CTX" = "1" ]; then
    _print_ctx >&2
  fi
  exit "$code"
}