#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
MODE="${MODE:-short}" # short | nightly

log() { printf "[alloc-ci] %s\n" "$*"; }
die() { printf "[alloc-ci][error] %s\n" "$*" >&2; exit 1; }

[ -x "$BIN" ] || die "missing binary: $BIN"

alloc_main="$ROOT_DIR/tests/modules/contracts/alloc/main.vit"
alloc_bench="$ROOT_DIR/tests/modules/contracts/alloc/main_bench.vit"
alloc_fuzz_short="$ROOT_DIR/tests/modules/contracts/alloc/main_fuzz_short.vit"
alloc_fuzz_nightly="$ROOT_DIR/tests/modules/contracts/alloc/main_fuzz_nightly.vit"
alloc_quality="$ROOT_DIR/tests/vitte_alloc_quality_matrix.vit"
alloc_integration="$ROOT_DIR/tests/vitte_alloc_actor_alerts_channel_fixture.vit"

check_one() {
  local src="$1"
  log "check $(realpath --relative-to="$ROOT_DIR" "$src")"
  local out
  out="$("$BIN" check --lang=en "$src" 2>&1)"
  grep -Fq "[driver] mir ok" <<<"$out" || die "fixture failed: $src"
}

log "mode=$MODE"
check_one "$alloc_main"
check_one "$alloc_bench"
check_one "$alloc_fuzz_short"
check_one "$alloc_integration"

if [ "$MODE" = "nightly" ]; then
  check_one "$alloc_fuzz_nightly"
  check_one "$alloc_quality"
fi

log "OK"
