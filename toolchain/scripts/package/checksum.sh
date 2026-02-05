#!/usr/bin/env bash
# ============================================================
# vitte — checksum utility
# Location: toolchain/scripts/package/checksum.sh
# ============================================================

set -euo pipefail

# ----------------------------
# Configuration
# ----------------------------
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/../../.." && pwd)}"
MODE="${MODE:-auto}"              # auto | gen | verify
ALGO="${ALGO:-sha256}"             # sha256 | sha512
TARGET="${TARGET:-}"               # file or directory
OUT_EXT="${OUT_EXT:-.sha256}"      # checksum file extension
VERBOSE="${VERBOSE:-0}"            # 0 | 1

# ----------------------------
# Helpers
# ----------------------------
log() { printf "[checksum] %s\n" "$*"; }
die() { printf "[checksum][error] %s\n" "$*" >&2; exit 1; }

run() {
  if [ "$VERBOSE" = "1" ]; then log "exec: $*"; fi
  "$@"
}

has() { command -v "$1" >/dev/null 2>&1; }

hash_cmd() {
  case "$ALGO" in
    sha256)
      if has sha256sum; then echo "sha256sum"
      elif has shasum; then echo "shasum -a 256"
      else die "no sha256 tool found"
      fi
      ;;
    sha512)
      if has sha512sum; then echo "sha512sum"
      elif has shasum; then echo "shasum -a 512"
      else die "no sha512 tool found"
      fi
      ;;
    *)
      die "unsupported ALGO: $ALGO"
      ;;
  esac
}

gen_file() {
  local f="$1"
  local cmd
  cmd="$(hash_cmd)"
  log "generating checksum for $f"
  (cd "$(dirname "$f")" && run $cmd "$(basename "$f")" > "$(basename "$f")$OUT_EXT")
}

verify_file() {
  local f="$1"
  local chk="$f$OUT_EXT"
  [ -f "$chk" ] || die "checksum file missing: $chk"
  local cmd
  cmd="$(hash_cmd)"
  log "verifying $f"
  (cd "$(dirname "$f")" && run $cmd -c "$(basename "$chk")")
}

# ----------------------------
# Init
# ----------------------------
cd "$ROOT_DIR"

[ -n "$TARGET" ] || die "TARGET not set (file or directory required)"

if [ "$MODE" = "auto" ]; then
  if [ -d "$TARGET" ]; then
    MODE="gen"
  else
    MODE="verify"
  fi
fi

log "mode=$MODE algo=$ALGO target=$TARGET"

# ----------------------------
# Directory mode
# ----------------------------
if [ -d "$TARGET" ]; then
  FILES=$(find "$TARGET" -type f ! -name "*$OUT_EXT" | sort)
  [ -n "$FILES" ] || die "no files found in $TARGET"

  for f in $FILES; do
    if [ "$MODE" = "gen" ]; then
      gen_file "$f"
    elif [ "$MODE" = "verify" ]; then
      verify_file "$f"
    else
      die "invalid MODE: $MODE"
    fi
  done

  log "directory $MODE complete"
  exit 0
fi

# ----------------------------
# Single file mode
# ----------------------------
[ -f "$TARGET" ] || die "target not found: $TARGET"

if [ "$MODE" = "gen" ]; then
  gen_file "$TARGET"
elif [ "$MODE" = "verify" ]; then
  verify_file "$TARGET"
else
  die "invalid MODE: $MODE"
fi

log "done"