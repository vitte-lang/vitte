#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
cd "$ROOT_DIR"

BIN="${BIN:-$ROOT_DIR/bin/vitte}"
OUT_DIR="$ROOT_DIR/build/repro"

log() { printf "[repro] %s\n" "$*"; }
die() { printf "[repro][error] %s\n" "$*" >&2; exit 1; }

[ -x "$BIN" ] || die "missing binary: $BIN (run make build)"
mkdir -p "$OUT_DIR"

cases=(min branch)
for name in "${cases[@]}"; do
  src="$ROOT_DIR/tests/repro/$name.vit"
  out1="$OUT_DIR/$name.1.vit"
  out2="$OUT_DIR/$name.2.vit"

  [ -f "$src" ] || die "missing vitte source: $src"

  log "$name: emit deterministic Vitte artifact twice"
  "$BIN" emit --stdout --repro "$src" > "$out1"
  "$BIN" emit --stdout --repro "$src" > "$out2"

  log "$name: compare emitted artifacts"
  cmp -s "$out1" "$out2" || die "$name: emitted artifacts differ"
done

log "all cases OK"
