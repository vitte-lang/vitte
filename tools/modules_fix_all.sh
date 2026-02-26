#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
ROOTS=(tests examples)
CHECK_ONLY=0
WRITE=1
BACKUP=1

log() { printf "[modules-fix-all] %s\n" "$*"; }
die() { printf "[modules-fix-all][error] %s\n" "$*" >&2; exit 1; }

usage() {
  cat <<'USAGE'
usage: tools/modules_fix_all.sh [--roots <dir> [<dir>...]] [--check] [--write] [--no-backup]

Options:
  --roots <dir>...   Roots to scan for *.vit (default: tests examples)
  --check            Dry-run: exit non-zero if a safe rewrite is available.
  --write            Apply rewrite using `mod doctor --fix --write` (default).
  --no-backup        Do not create .bak before writing.
USAGE
}

while [ $# -gt 0 ]; do
  case "$1" in
    --roots)
      ROOTS=()
      shift
      while [ $# -gt 0 ] && [[ "$1" != --* ]]; do
        ROOTS+=("$1")
        shift
      done
      continue
      ;;
    --roots=*)
      ROOTS=()
      IFS=',' read -r -a split_roots <<<"${1#--roots=}"
      for r in "${split_roots[@]}"; do
        [ -n "$r" ] && ROOTS+=("$r")
      done
      ;;
    --check) CHECK_ONLY=1 ;;
    --write) WRITE=1 ;;
    --no-backup) BACKUP=0 ;;
    -h|--help) usage; exit 0 ;;
    *) die "unknown option: $1" ;;
  esac
  shift
done

[ -x "$BIN" ] || die "missing binary: $BIN"

scanned=0
candidates=0
rewritten=0
for root in "${ROOTS[@]}"; do
  scan_root="$ROOT_DIR/$root"
  [ -d "$scan_root" ] || continue
  while IFS= read -r file; do
    scanned=$((scanned + 1))
    set +e
    out="$($BIN mod doctor --lang=en --fix "$file" 2>&1)"
    rc=$?
    set -e
    : "$rc"

    if ! grep -Fq "legacy import path in" <<<"$out"; then
      continue
    fi

    candidates=$((candidates + 1))
    rel="$file"
    case "$file" in
      "$ROOT_DIR"/*) rel="${file#"$ROOT_DIR"/}" ;;
    esac

    if [ "$CHECK_ONLY" -eq 1 ]; then
      log "candidate: $rel"
      continue
    fi

    if [ "$WRITE" -eq 1 ]; then
      if [ "$BACKUP" -eq 1 ]; then
        cp "$file" "$file.bak"
      fi
      set +e
      out_write="$($BIN mod doctor --lang=en --fix --write "$file" 2>&1)"
      rc_write=$?
      set -e
      : "$rc_write"
      if grep -Fq "[doctor] write: rewrote" <<<"$out_write"; then
        rewritten=$((rewritten + 1))
        log "rewrote: $rel"
      else
        if [ "$BACKUP" -eq 1 ] && [ -f "$file.bak" ]; then
          rm -f "$file.bak"
        fi
      fi
    fi
  done < <(find "$scan_root" -type f -name '*.vit' | sort)
done

log "scanned=$scanned candidates=$candidates rewritten=$rewritten"
if [ "$CHECK_ONLY" -eq 1 ] && [ "$candidates" -gt 0 ]; then
  die "rewrite candidates found"
fi
