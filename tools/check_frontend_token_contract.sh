#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT_DIR"

FRONTEND_ROOT="src/vitte/compiler/frontend"
TEST_ROOT="src/vitte/compiler/tests"

forbidden_import_hits=$(
  rg -n 'frontend/lex/' "$FRONTEND_ROOT" "$TEST_ROOT" 2>/dev/null || true
)

if [ -n "$forbidden_import_hits" ]; then
  echo "[frontend-token-contract][error] forbidden legacy frontend import path found:" >&2
  echo "$forbidden_import_hits" >&2
  exit 1
fi

forbidden_kind_hits=$(
  rg -n 'TokenKind\.(Newline|Comment|Identifier|LeftBrace|RightBrace|LeftParen|RightParen|LeftBracket|RightBracket|Colon|Comma|Semicolon|At|Hash|Arrow|FatArrow)\b' \
    "$FRONTEND_ROOT" "$TEST_ROOT" 2>/dev/null || true
)

if [ -n "$forbidden_kind_hits" ]; then
  echo "[frontend-token-contract][error] forbidden legacy token kinds found:" >&2
  echo "$forbidden_kind_hits" >&2
  exit 1
fi

echo "[frontend-token-contract] ok"
