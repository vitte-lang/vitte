#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"

log() { printf "[strict-recovery-smoke] %s\n" "$*"; }
die() { printf "[strict-recovery-smoke][error] %s\n" "$*" >&2; exit 1; }

[ -x "$BIN" ] || die "missing binary: $BIN"

tmp="$(mktemp "${TMPDIR:-/tmp}/vitte-strict-recovery-XXXXXX.vit")"
cat >"$tmp" <<'VIT'
if true {
  return 1
}
VIT

set +e
out="$($BIN parse --parse-silent --strict-recovery=0 "$tmp" 2>&1)"
rc=$?
set -e
rm -f "$tmp"

if [ "$rc" -eq 0 ]; then
  printf '%s\n' "$out"
  die "expected non-zero exit when recoveries exceed strict-recovery=0"
fi

printf '%s\n' "$out" | grep -q "parser recoveries" || die "missing strict-recovery failure message"
log "OK"
