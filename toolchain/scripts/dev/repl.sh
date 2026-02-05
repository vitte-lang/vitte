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

mkdir -p "$TMP_DIR"

# ----------------------------
# Helpers
# ----------------------------
log() { printf "[repl] %s\n" "$*"; }
die() { printf "[repl][error] %s\n" "$*" >&2; exit 1; }

has() { command -v "$1" >/dev/null 2>&1; }

need_vittec() {
  if [ ! -x "$VITTEC_BIN" ]; then
    die "vittec not found or not executable: $VITTEC_BIN"
  fi
}

banner() {
  cat <<'EOF'
Vitte REPL
Commands:
  :help            show this help
  :quit | :exit    leave repl
  :mode <m>        set mode (exec|check|ast)
  :edit            open editor with last buffer
  :clear           clear current buffer
  :load <file>     load file into buffer
  :save <file>     save buffer to file
  :run             run current buffer
  :show            show current buffer
EOF
}

run_buffer() {
  local src="$1"
  local out="$TMP_DIR/out"
  case "$MODE" in
    exec)
      "$VITTEC_BIN" run "$src"
      ;;
    check)
      "$VITTEC_BIN" check "$src"
      ;;
    ast)
      "$VITTEC_BIN" ast "$src"
      ;;
    *)
      die "unknown mode: $MODE"
      ;;
  esac
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