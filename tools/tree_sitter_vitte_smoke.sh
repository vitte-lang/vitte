#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="${ROOT_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
TS_DIR="$ROOT_DIR/editors/tree-sitter"
CORPUS="$TS_DIR/test/corpus/core.txt"

if ! command -v tree-sitter >/dev/null 2>&1; then
  echo "[tree-sitter-vitte-smoke] skip: tree-sitter CLI not installed"
  exit 0
fi

(
  cd "$TS_DIR"
  tree-sitter generate >/tmp/vitte-tree-sitter-generate.out 2>&1 || { cat /tmp/vitte-tree-sitter-generate.out >&2 || true; echo "[tree-sitter-vitte-smoke][error] generate failed" >&2; exit 1; }
  tree-sitter test >/tmp/vitte-tree-sitter-test.out 2>&1 || { cat /tmp/vitte-tree-sitter-test.out >&2 || true; echo "[tree-sitter-vitte-smoke][error] test failed" >&2; exit 1; }
)

echo "[tree-sitter-vitte-smoke] OK"
