#!/usr/bin/env bash
# ============================================================
# vitte — logging utility
# Location: toolchain/scripts/utils/log.sh
# ============================================================

# Usage:
#   source toolchain/scripts/utils/log.sh
#   log_info "message"
#   log_warn "message"
#   log_error "message"
#   log_debug "message"

set -euo pipefail

# ----------------------------
# Configuration
# ----------------------------
LOG_PREFIX="${LOG_PREFIX:-[vitte]}"
LOG_LEVEL="${LOG_LEVEL:-info}"        # debug | info | warn | error
LOG_TIME="${LOG_TIME:-1}"             # 1: show timestamp
LOG_COLOR="${LOG_COLOR:-auto}"        # auto | always | never

# ----------------------------
# Helpers
# ----------------------------
_has_tty() { [ -t 1 ]; }

_color_enabled() {
  case "$LOG_COLOR" in
    always) return 0 ;;
    never)  return 1 ;;
    auto)   _has_tty ;;
    *)      return 1 ;;
  esac
}

_ts() {
  if [ "$LOG_TIME" = "1" ]; then
    date +"%Y-%m-%d %H:%M:%S"
  fi
}

_level_rank() {
  case "$1" in
    debug) echo 0 ;;
    info)  echo 1 ;;
    warn)  echo 2 ;;
    error) echo 3 ;;
    *)     echo 1 ;;
  esac
}

_should_log() {
  local want="$(_level_rank "$1")"
  local cur="$(_level_rank "$LOG_LEVEL")"
  [ "$want" -ge "$cur" ]
}

_emit() {
  local level="$1" color="$2" msg="$3"
  local ts="$(_ts)"

  if _color_enabled; then
    printf "%s %b%s%b %s\n" \
      "${ts:+[$ts]}" \
      "$color" "$LOG_PREFIX/$level" "\033[0m" \
      "$msg"
  else
    printf "%s %s/%s %s\n" \
      "${ts:+[$ts]}" \
      "$LOG_PREFIX" "$level" \
      "$msg"
  fi
}

# ----------------------------
# Public API
# ----------------------------
log_debug() {
  _should_log debug || return 0
  _emit debug "\033[36m" "$*"
}

log_info() {
  _should_log info || return 0
  _emit info "\033[32m" "$*"
}

log_warn() {
  _should_log warn || return 0
  _emit warn "\033[33m" "$*" >&2
}

log_error() {
  _should_log error || return 0
  _emit error "\033[31m" "$*" >&2
}