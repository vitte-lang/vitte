# =========================================================
# Vitte — Core (Python)
# =========================================================
"""
Noyau Python de Vitte.

Rôle :
- orchestration du pipeline : lexer → parser → AST → HIR → validation
- point d’entrée stable pour outils (tests, LSP, bootstrap)
- aucune génération de code machine ici
"""

from dataclasses import dataclass
from typing import Optional

from .config import CoreConfig
from .errors import DiagnosticBag
from .span import Span
from .lexer import lex
from .parser import parse
from .ast import AstModule
from .hir import HirModule
from .validate import validate


# ---------------------------------------------------------
# Résultats
# ---------------------------------------------------------

@dataclass
class CoreResult:
    ok: bool
    diagnostics: DiagnosticBag
    ast: Optional[AstModule] = None
    hir: Optional[HirModule] = None


# ---------------------------------------------------------
# Session Core
# ---------------------------------------------------------

@dataclass
class CoreSession:
    config: CoreConfig
    diagnostics: DiagnosticBag

    def error(self, msg: str, span: Optional[Span] = None) -> None:
        self.diagnostics.error(msg, span)

    def warning(self, msg: str, span: Optional[Span] = None) -> None:
        self.diagnostics.warning(msg, span)


# ---------------------------------------------------------
# Pipeline principal
# ---------------------------------------------------------

def run_source(
    source: str,
    config: Optional[CoreConfig] = None
) -> CoreResult:
    """
    Exécute le pipeline complet sur une source Vitte.
    """
    cfg = config or CoreConfig.default()
    diags = DiagnosticBag()
    session = CoreSession(cfg, diags)

    # --- Lexer ---
    tokens = lex(source, diags)
    if diags.has_errors():
        return CoreResult(False, diags)

    if cfg.dump_tokens:
        diags.note("tokens dumped")
        for t in tokens:
            print(t)

    # --- Parser (AST) ---
    ast = parse(tokens, diags)
    if diags.has_errors() or ast is None:
        return CoreResult(False, diags)

    if cfg.emit_ast:
        print(ast)

    # --- Lowering AST → HIR ---
    hir = HirModule.from_ast(ast, diags)
    if diags.has_errors():
        return CoreResult(False, diags, ast=ast)

    if cfg.dump_hir:
        print(hir)

    # --- Validation sémantique ---
    if cfg.validate:
        validate(hir, diags)
        if diags.has_errors():
            return CoreResult(False, diags, ast=ast, hir=hir)

    return CoreResult(True, diags, ast=ast, hir=hir)


# ---------------------------------------------------------
# Helpers
# ---------------------------------------------------------

def run_file(
    path: str,
    config: Optional[CoreConfig] = None
) -> CoreResult:
    """
    Charge un fichier et exécute le pipeline Core.
    """
    with open(path, "r", encoding="utf-8") as f:
        source = f.read()
    return run_source(source, config)


def check_source(
    source: str,
    config: Optional[CoreConfig] = None
) -> bool:
    """
    Vérifie uniquement (syntaxe + sémantique).
    """
    res = run_source(source, config)
    return res.ok


def parse_only(source: str) -> Optional[AstModule]:
    """
    Parsing AST seul (utile pour tests).
    """
    diags = DiagnosticBag()
    tokens = lex(source, diags)
    if diags.has_errors():
        return None
    return parse(tokens, diags)