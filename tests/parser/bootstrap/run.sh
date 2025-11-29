#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/out"
SRC_DIR="$ROOT/tests/parser/bootstrap"
TMP="$OUT/parser-bootstrap"
mkdir -p "$TMP"

PARSER_SCRIPT="$ROOT/samples/parser_min/main.vitte"
if [ ! -f "$PARSER_SCRIPT" ]; then
    cat <<'SCRIPT' > "$TMP/parser_entry.vitte"
module parser.entry:
    import bootstrap.parser.lex as lex
    import bootstrap.parser.parse as parse
    import json
    scenario run_file(path):
        let source = fs.read(path)
        let tokens = lex.scan_source(source)
        let ast = parse.parse_module(tokens)
        io.write(json.stringify(ast, pretty=true))
SCRIPT
    PARSER_SCRIPT="$TMP/parser_entry.vitte"
fi

if [ ! -x "$OUT/vbc-run" ]; then
    echo "[parser-bootstrap] out/vbc-run absent; impossible d'exécuter parser" >&2
    exit 1
fi
if [ ! -f "$OUT/vittec.vbc" ]; then
    echo "[parser-bootstrap] out/vittec.vbc manquant" >&2
    exit 1
fi

status=0
for file in "$SRC_DIR"/*.vitte; do
    name=$(basename "$file")
    log="$TMP/${name%.vitte}.json"
    echo "[parser-bootstrap] $name"
    if ! "$OUT/vbc-run" "$OUT/vittec.vbc" "$PARSER_SCRIPT" "$file" > "$log"; then
        echo "  ❌ échec" >&2
        status=1
        continue
    fi
    echo "  ✅ AST -> $log"
done
exit $status
