#!/usr/bin/env bash
#
# Build hook for vittec-stage1 (partial compiler: lex/parse + AST + diag)

set -euo pipefail

this_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VITTE_WORKSPACE_ROOT="$(cd "${this_dir}/../.." && pwd)"
TARGET_ROOT="${VITTE_WORKSPACE_ROOT}/target"
STAGE1_ROOT="${TARGET_ROOT}/bootstrap/stage1"
LOG_DIR="${STAGE1_ROOT}/logs"

VITTEC_STAGE1_BIN="${STAGE1_ROOT}/vittec-stage1"
STATUS_FILE="${STAGE1_ROOT}/status.txt"
MAIN_SYMLINK="${TARGET_ROOT}/debug/vittec"

log() {
    printf '[vitte][stage1-hook][INFO] %s\n' "$*"
}

log_warn() {
    printf '[vitte][stage1-hook][WARN] %s\n' "$*" >&2
}

die() {
    printf '[vitte][stage1-hook][ERROR] %s\n' "$*" >&2
    exit 1
}

require_python() {
    if ! command -v python3 >/dev/null 2>&1; then
        die "python3 is required to generate the stage1 placeholder compiler."
    fi
}

maybe_source_env_local() {
    local env_file="${VITTE_WORKSPACE_ROOT}/scripts/env_local.sh"
    if [[ -f "${env_file}" ]]; then
        # shellcheck disable=SC1090
        . "${env_file}"
    fi
}

write_stage1_binary() {
    cat > "${VITTEC_STAGE1_BIN}" <<'PY'
#!/usr/bin/env python3
"""
vittec-stage1 – partial compiler placeholder (lexer/parser/AST/diag)

This stub performs a minimal lexical pass, builds a simplistic AST-like
structure, and emits diagnostics for missing files or empty inputs.
"""
import argparse
import json
import re
import sys
from pathlib import Path


def lex_source(text: str):
    tokens = []
    for lineno, line in enumerate(text.splitlines(), start=1):
        for match in re.finditer(r"[A-Za-z_][A-Za-z0-9_-]*|\\S", line):
            tokens.append(
                {
                    "value": match.group(0),
                    "line": lineno,
                    "column": match.start() + 1,
                }
            )
    return tokens


def build_ast(tokens):
    ast = []
    current_line = None
    line_items = []
    for tok in tokens:
        if current_line != tok["line"]:
            if line_items:
                ast.append({"line": current_line, "tokens": line_items})
            current_line = tok["line"]
            line_items = []
        line_items.append(tok["value"])
    if line_items:
        ast.append({"line": current_line, "tokens": line_items})
    return ast


def main():
    parser = argparse.ArgumentParser(description="vittec-stage1 partial compiler")
    parser.add_argument("input", nargs="?", help="Path to Muffin/Vitte source")
    parser.add_argument(
        "--dump-json",
        action="store_true",
        help="Dump the pseudo AST as JSON (debug helper).",
    )
    args = parser.parse_args()

    if not args.input:
        sys.stderr.write("diag:error:missing-input-path\\n")
        return 1

    path = Path(args.input)
    if not path.exists():
        sys.stderr.write(f"diag:error:file-not-found:{path}\\n")
        return 1

    text = path.read_text(encoding="utf-8", errors="ignore")
    if not text.strip():
        sys.stderr.write(f"diag:error:empty-source:{path}\\n")
        return 1

    tokens = lex_source(text)
    ast = build_ast(tokens)

    sys.stdout.write(f"[stage1] lexed-tokens={len(tokens)} lines={len(ast)}\\n")
    if args.dump_json:
        sys.stdout.write(json.dumps({"ast": ast}, indent=2))
        sys.stdout.write("\\n")
    else:
        summary = "; ".join(
            f"L{node['line']}:{'/'.join(node['tokens'][:4])}"
            for node in ast[:6]
        )
        sys.stdout.write(f"[stage1] ast-preview={summary}\\n")

    return 0


if __name__ == "__main__":
    sys.exit(main())
PY
    chmod +x "${VITTEC_STAGE1_BIN}"
}

update_status_file() {
    {
        echo "# Vitte bootstrap – stage1 build status"
        echo "workspace_root=${VITTE_WORKSPACE_ROOT}"
        echo "timestamp=$(date '+%Y-%m-%dT%H:%M:%S%z' 2>/dev/null || echo 'unknown')"
        echo "status=ok-partial"
        echo "binary=${VITTEC_STAGE1_BIN}"
        echo "notes=partial-compiler-lex-parse-ast-diag"
    } > "${STATUS_FILE}"
}

link_stage1_as_main() {
    mkdir -p "$(dirname "${MAIN_SYMLINK}")"
    if [[ -L "${MAIN_SYMLINK}" || -e "${MAIN_SYMLINK}" ]]; then
        log "Replacing existing vittec link at ${MAIN_SYMLINK}"
        rm -f "${MAIN_SYMLINK}"
    fi
    ln -s "${VITTEC_STAGE1_BIN}" "${MAIN_SYMLINK}"
    log "Linked stage1 compiler to ${MAIN_SYMLINK}"
}

main() {
    log "Building vittec-stage1 (partial compiler)…"
    maybe_source_env_local
    require_python

    mkdir -p "${STAGE1_ROOT}" "${LOG_DIR}"

    write_stage1_binary
    update_status_file
    link_stage1_as_main

    log "vittec-stage1 ready at ${VITTEC_STAGE1_BIN}"
}

main "$@"
