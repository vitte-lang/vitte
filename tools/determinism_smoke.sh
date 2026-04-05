#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
BIN="${BIN:-$ROOT_DIR/bin/vitte}"
SRC="${SRC:-$ROOT_DIR/examples/first_project.vit}"

log() { printf "[determinism-smoke] %s\n" "$*"; }
die() { printf "[determinism-smoke][error] %s\n" "$*" >&2; exit 1; }

[ -x "$BIN" ] || die "missing binary: $BIN"
[ -f "$SRC" ] || die "missing source: $SRC"

t1="$(mktemp "${TMPDIR:-/tmp}/vitte-det-ast1-XXXXXX")"
t2="$(mktemp "${TMPDIR:-/tmp}/vitte-det-ast2-XXXXXX")"
h1="$(mktemp "${TMPDIR:-/tmp}/vitte-det-hir1-XXXXXX")"
h2="$(mktemp "${TMPDIR:-/tmp}/vitte-det-hir2-XXXXXX")"
trap 'rm -f "$t1" "$t2" "$h1" "$h2"' EXIT

"$BIN" parse --parse-silent --deterministic --dump-ast-json "$SRC" >"$t1" 2>/dev/null
"$BIN" parse --parse-silent --deterministic --dump-ast-json "$SRC" >"$t2" 2>/dev/null

diff -u "$t1" "$t2" >/dev/null || die "AST deterministic output mismatch"

"$BIN" check --deterministic --dump-hir-json "$SRC" >"$h1" 2>/dev/null || true
"$BIN" check --deterministic --dump-hir-json "$SRC" >"$h2" 2>/dev/null || true

diff -u "$h1" "$h2" >/dev/null || die "HIR deterministic output mismatch"

log "OK"
