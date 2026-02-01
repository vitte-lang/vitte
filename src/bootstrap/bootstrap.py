

# =========================================================
# Vitte — Bootstrap (Python)
# =========================================================
"""
Bootstrap Python pour le langage Vitte.

Rôle :
- point d’entrée minimal et robuste pour le noyau Python de Vitte
- utilisé pour tests, tooling, LSP, prototypage
- prépare la transition vers le compilateur natif (vittec)

Ce bootstrap :
- ne génère PAS de code machine
- ne dépend PAS du backend
- orchestre uniquement le frontend (core)
"""

from __future__ import annotations

import sys
import argparse
from pathlib import Path
from typing import Optional

# Import noyau Vitte
from core.core import (
    run_source,
    run_file,
    check_source,
    CoreResult,
)
from core.config import CoreConfig
from core.errors import DiagnosticBag


# ---------------------------------------------------------
# Configuration CLI bootstrap
# ---------------------------------------------------------

def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="vitte-bootstrap",
        description="Bootstrap Python pour le langage Vitte",
    )

    parser.add_argument(
        "input",
        nargs="?",
        help="Fichier .vit à analyser",
    )

    parser.add_argument(
        "--check",
        action="store_true",
        help="Analyse uniquement (pas d'affichage AST/HIR)",
    )

    parser.add_argument(
        "--emit-ast",
        action="store_true",
        help="Affiche l'AST",
    )

    parser.add_argument(
        "--dump-hir",
        action="store_true",
        help="Affiche le HIR",
    )

    parser.add_argument(
        "--dump-tokens",
        action="store_true",
        help="Affiche les tokens du lexer",
    )

    parser.add_argument(
        "--no-validate",
        action="store_true",
        help="Désactive la validation sémantique",
    )

    return parser


# ---------------------------------------------------------
# Mapping CLI → CoreConfig
# ---------------------------------------------------------

def make_config(args: argparse.Namespace) -> CoreConfig:
    cfg = CoreConfig.default()

    cfg.emit_ast = bool(args.emit_ast)
    cfg.dump_hir = bool(args.dump_hir)
    cfg.dump_tokens = bool(args.dump_tokens)
    cfg.validate = not bool(args.no_validate)

    return cfg


# ---------------------------------------------------------
# Exécution bootstrap
# ---------------------------------------------------------

def run(args: argparse.Namespace) -> int:
    cfg = make_config(args)

    if args.input is None:
        print("no input file")
        return 1

    path = Path(args.input)

    if not path.exists():
        print(f"file not found: {path}")
        return 1

    if path.suffix != ".vit":
        print("expected a .vit file")
        return 1

    if args.check:
        ok = check_source(path.read_text(encoding="utf-8"), cfg)
        return 0 if ok else 1

    result: CoreResult = run_file(str(path), cfg)

    if not result.ok:
        print("errors:")
        for d in result.diagnostics:
            print(d)
        return 1

    return 0


# ---------------------------------------------------------
# Entrée principale
# ---------------------------------------------------------

def main(argv: Optional[list[str]] = None) -> int:
    parser = build_arg_parser()
    args = parser.parse_args(argv)
    return run(args)


if __name__ == "__main__":
    sys.exit(main())