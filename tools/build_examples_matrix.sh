#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
MODE="${MODE:-build}"  # build | check

log() { printf "[examples-matrix] %s\n" "$*"; }
die() { printf "[examples-matrix][error] %s\n" "$*" >&2; exit 1; }

[ -x "$BIN" ] || die "missing binary: $BIN"
[ -d "$ROOT_DIR/examples" ] || die "missing examples dir"

case "$MODE" in
  build|check) ;;
  *) die "invalid MODE=$MODE (expected build or check)" ;;
esac

total=0
ok=0
fail=0

while IFS= read -r file; do
  [ -n "$file" ] || continue
  total=$((total + 1))
  rel="${file#$ROOT_DIR/}"
  log "$MODE: $rel"
  if "$BIN" "$MODE" "$file" >/tmp/vitte_examples_matrix.log 2>&1; then
    ok=$((ok + 1))
  else
    fail=$((fail + 1))
    printf "[examples-matrix][fail] %s\n" "$rel" >&2
    sed -n '1,80p' /tmp/vitte_examples_matrix.log >&2
  fi
done <<EOF_FILES
$(find "$ROOT_DIR/examples" -maxdepth 1 -type f -name '*.vit' | sort)
EOF_FILES

[ "$total" -gt 0 ] || die "no .vit files in examples/"
log "summary: total=$total ok=$ok fail=$fail"
[ "$fail" -eq 0 ]
