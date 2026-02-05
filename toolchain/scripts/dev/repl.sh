#!/usr/bin/env bash
# ============================================================
# vitte — developer REPL
# Location: toolchain/scripts/dev/repl.sh
# ============================================================

set -euo pipefail

# ----------------------------
# Configuration
# ----------------------------
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"
PROMPT="${PROMPT:-vitte> }"
EDITOR_CMD="${EDITOR_CMD:-${EDITOR:-vi}}"
TMP_DIR="${TMP_DIR:-$ROOT_DIR/target/repl}"
VITTEC_BIN="${VITTEC_BIN:-$ROOT_DIR/target/bin/vittec}"
MODE="${MODE:-exec}"     # exec | check | ast (if supported)
HISTORY_FILE="${HISTORY_FILE:-$HOME/.vitte_repl_history}"
HASH_FILE="$TMP_DIR/.last_hash"
FALLBACK_MODE="${FALLBACK_MODE:-check}" # check | parse | none

mkdir -p "$TMP_DIR"

# ----------------------------
# Helpers
# ----------------------------
log() { printf "[repl] %s\n" "$*"; }
die() { printf "[repl][error] %s\n" "$*" >&2; exit 1; }

has() { command -v "$1" >/dev/null 2>&1; }

hash_cmd() {
  if has sha256sum; then
    echo "sha256sum"
  elif has shasum; then
    echo "shasum -a 256"
  else
    return 1
  fi
}

file_hash() {
  local f="$1"
  local cmd
  cmd="$(hash_cmd)" || return 1
  $cmd "$f" | awk '{print $1}'
}

need_vittec() {
  if [ -x "$VITTEC_BIN" ]; then
    return
  fi

  if [ -x "$ROOT_DIR/bin/vittec" ]; then
    VITTEC_BIN="$ROOT_DIR/bin/vittec"
    return
  fi

  if [ -x "$ROOT_DIR/bin/vitte" ]; then
    VITTEC_BIN="$ROOT_DIR/bin/vitte"
    return
  fi

  die "vittec not found or not executable: $VITTEC_BIN"
}

banner() {
  cat <<'EOF'
Vitte REPL
Commands:
  :help            show this help
  :quit | :exit    leave repl
  :mode <m>        set mode (exec|check|ast)
  :fallback <m>    set fallback (check|parse|none)
  :edit            open editor with last buffer
  :clear           clear current buffer
  :load <file>     load file into buffer
  :save <file>     save buffer to file
  :run             run current buffer
  :show            show current buffer
  :cpp             compile and show generated C++
  :reset           clear buffer + cache artifacts
Notes:
  If no `entry` is present, the buffer is wrapped into a default entry.
  If `print` is detected, an implicit `pull core/io as io` is added.
  On exec failure, falls back to check/parse (FALLBACK_MODE=check|parse|none).
EOF
}

prepare_buffer() {
  local src="$1"
  local prepared="$src"

  if ! grep -Eq "^[[:space:]]*entry\\b" "$src"; then
    prepared="$TMP_DIR/wrapped.vit"
    local need_io="0"
    if grep -Eq "\\bprint\\b" "$src" && ! grep -Eq "^[[:space:]]*pull[[:space:]]+core/io\\b" "$src"; then
      need_io="1"
    fi
    {
      printf "space core/app\n"
      if [ "$need_io" = "1" ]; then
        printf "pull core/io as io\n"
      fi
      printf "entry main at core/app {\n"
      sed 's/^/  /' "$src"
      printf "}\n"
    } > "$prepared"
  fi

  printf "%s" "$prepared"
}

run_buffer() {
  local src="$1"
  local out="$TMP_DIR/out"
  local prepared
  prepared="$(prepare_buffer "$src")"
  local build_log="$TMP_DIR/build.log"

  case "$MODE" in
    exec)
      local current_hash=""
      if current_hash="$(file_hash "$prepared" 2>/dev/null)"; then
        if [ -f "$HASH_FILE" ] && [ -x "$out" ]; then
          local last_hash
          last_hash="$(cat "$HASH_FILE" 2>/dev/null || true)"
          if [ -n "$last_hash" ] && [ "$last_hash" = "$current_hash" ]; then
            "$out"
            return
          fi
        fi
      fi

      if ! "$VITTEC_BIN" build -o "$out" "$prepared" >"$build_log" 2>&1; then
        if [ "$FALLBACK_MODE" = "check" ]; then
          log "exec failed; falling back to check"
          if [ "${VERBOSE:-0}" = "1" ]; then
            log "exec output:"
            sed -n '1,200p' "$build_log"
          fi
          "$VITTEC_BIN" check "$prepared"
        elif [ "$FALLBACK_MODE" = "parse" ]; then
          log "exec failed; falling back to parse"
          if [ "${VERBOSE:-0}" = "1" ]; then
            log "exec output:"
            sed -n '1,200p' "$build_log"
          fi
          "$VITTEC_BIN" parse "$prepared"
        fi
        return
      fi
      [ -n "$current_hash" ] && echo "$current_hash" > "$HASH_FILE"
      "$out"
      ;;
    check)
      "$VITTEC_BIN" check "$prepared"
      ;;
    ast)
      "$VITTEC_BIN" parse --dump-ast "$prepared"
      ;;
    *)
      die "unknown mode: $MODE"
      ;;
  esac
}

show_cpp() {
  local prepared="$1"
  local out="$TMP_DIR/out"
  local log="$TMP_DIR/build.log"

  if ! "$VITTEC_BIN" build -o "$out" "$prepared" >"$log" 2>&1; then
    log "emit failed; see build log with VERBOSE=1"
    if [ "${VERBOSE:-0}" = "1" ]; then
      sed -n '1,200p' "$log"
    fi
    return 1
  fi

  if [ -f "$ROOT_DIR/vitte_out.cpp" ]; then
    sed -n '1,200p' "$ROOT_DIR/vitte_out.cpp"
  else
    log "vitte_out.cpp not found"
    return 1
  fi
}

reset_repl() {
  : > "$BUFFER_FILE"
  rm -f "$TMP_DIR/out" "$HASH_FILE" "$TMP_DIR/wrapped.vit" "$TMP_DIR/build.log" || true
  log "reset done"
}

# ----------------------------
# Init
# ----------------------------
cd "$ROOT_DIR"
need_vittec

BUFFER_FILE="$TMP_DIR/session.vit"
: > "$BUFFER_FILE"

banner

# readline history (best-effort)
if has rlwrap; then
  RL="rlwrap -H $HISTORY_FILE"
else
  RL=""
fi

# ----------------------------
# Loop
# ----------------------------
while true; do
  if [ -n "$RL" ]; then
    read -r -p "$PROMPT" line || break
  else
    printf "%s" "$PROMPT"
    read -r line || break
  fi

  case "$line" in
    :quit|:exit)
      break
      ;;
    :help)
      banner
      ;;
    :mode*)
      MODE="${line#*:mode }"
      log "mode=$MODE"
      ;;
    :fallback*)
      FALLBACK_MODE="${line#*:fallback }"
      log "fallback=$FALLBACK_MODE"
      ;;
    :edit)
      "$EDITOR_CMD" "$BUFFER_FILE"
      ;;
    :clear)
      : > "$BUFFER_FILE"
      log "buffer cleared"
      ;;
    :load*)
      f="${line#*:load }"
      [ -f "$f" ] || die "file not found: $f"
      cp "$f" "$BUFFER_FILE"
      log "loaded $f"
      ;;
    :save*)
      f="${line#*:save }"
      cp "$BUFFER_FILE" "$f"
      log "saved to $f"
      ;;
    :show)
      sed -n '1,200p' "$BUFFER_FILE"
      ;;
    :cpp)
      show_cpp "$(prepare_buffer "$BUFFER_FILE")"
      ;;
    :reset)
      reset_repl
      ;;
    :run)
      run_buffer "$BUFFER_FILE"
      ;;
    :*)
      die "unknown command: $line"
      ;;
    *)
      # append code line
      printf "%s\n" "$line" >> "$BUFFER_FILE"
      ;;
  esac
done

log "bye"
