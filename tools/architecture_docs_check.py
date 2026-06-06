#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parent.parent
DOCS = [
    ROOT / "docs" / "compiler" / "architecture.md",
    ROOT / "docs" / "compiler" / "pipeline.md",
    ROOT / "docs" / "COMPILER_DRIVER_MIGRATION.md",
    ROOT / "docs" / "bootstrap_contracts.md",
    ROOT / "docs" / "bootstrap_migration_checklist.md",
    ROOT / "docs" / "MAKE_TARGETS.md",
]

FORBIDDEN = [
    "src/vitte/compiler/frontend/parser.vit",
    "src/vitte/compiler/frontend/lexer.vit",
    "src/vitte/compiler/frontend/token.vit",
    "src/vitte/compiler/frontend/expr_parser.vit",
    "src/vitte/compiler/frontend/diagnostics.vit",
    "ast_extended.vit",
    "hir_extended.vit",
    "architecture_integration_tests.vit",
    "C backend code generation",
    "Code Generation (C backend)",
    "generating .c/.h",
]

REQUIRED = [
    "toolchain/seed/vittec0.seed",
    "make bootstrap-native-contract",
    "make bootstrap-verify",
    "make build",
]


def main() -> int:
    failed = False
    combined = ""
    for doc in DOCS:
        if not doc.is_file():
            print(f"[architecture-docs][error] missing documentation file `{doc.relative_to(ROOT)}`", file=sys.stderr)
            failed = True
            continue
        text = doc.read_text(encoding="utf-8")
        combined += text + "\n"
        for forbidden in FORBIDDEN:
            if forbidden in text:
                print(f"[architecture-docs][error] {doc.name}: stale reference `{forbidden}`", file=sys.stderr)
                failed = True
    for required in REQUIRED:
        if required not in combined:
            print(f"[architecture-docs][error] missing required reference `{required}`", file=sys.stderr)
            failed = True
    if failed:
        return 1
    print("[architecture-docs] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
