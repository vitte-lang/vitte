#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
MANIFEST="${MANIFEST:-}"
log() { printf "[check-manifest] %s\n" "$*"; }
die() { printf "[check-manifest][error] %s\n" "$*" >&2; exit 1; }

[ -x "$BIN" ] || die "missing binary: $BIN"
[ -n "$MANIFEST" ] || die "MANIFEST is required"

manifest_path="$MANIFEST"
case "$manifest_path" in
  /*) ;;
  *) manifest_path="$ROOT_DIR/$manifest_path" ;;
esac
[ -f "$manifest_path" ] || die "missing manifest: $manifest_path"

files=()
while IFS= read -r rel; do
  rel="${rel#"${rel%%[![:space:]]*}"}"
  rel="${rel%"${rel##*[![:space:]]}"}"
  [ -z "$rel" ] && continue
  case "$rel" in
    \#*) continue ;;
  esac
  case "$rel" in
    /*) f="$rel" ;;
    *) f="$ROOT_DIR/$rel" ;;
  esac
  [ -f "$f" ] || die "manifest entry missing file: $rel"
  files+=("$f")
done < "$manifest_path"

[ "${#files[@]}" -gt 0 ] || die "manifest is empty: $manifest_path"

for src in "${files[@]}"; do
  rel="$src"
  case "$src" in
    "$ROOT_DIR"/*) rel="${src#"$ROOT_DIR"/}" ;;
  esac
  base="${src%.vit}"
  cmd="check"
  [ -f "$base.cmd" ] && cmd="$(tr -d '\n' < "$base.cmd")"
  flags=""
  [ -f "$base.flags" ] && flags="$(cat "$base.flags")"
  log "$rel"
  set +e
  out="$("$BIN" "$cmd" --lang=en $flags "$rel" 2>&1)"
  rc=$?
  set -e
  if [ "$rc" -ne 0 ]; then
    printf "%s\n" "$out"
    die "unexpected exit code for $rel (expected=0 got=$rc)"
  fi
done

log "OK"
