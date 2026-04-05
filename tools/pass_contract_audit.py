#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]


def require_sequence(text: str, needles: list[str], label: str) -> list[str]:
    errors: list[str] = []
    pos = 0
    for n in needles:
        i = text.find(n, pos)
        if i < 0:
            errors.append(f"{label}: missing sequence token: {n}")
            continue
        pos = i + len(n)
    return errors


def main() -> int:
    pipeline = (ROOT / "src/compiler/driver/pipeline.cpp").read_text(encoding="utf-8")
    passes = (ROOT / "src/compiler/driver/passes.cpp").read_text(encoding="utf-8")

    errs: list[str] = []

    # Stage ordering contract in pipeline.
    errs.extend(require_sequence(
        pipeline,
        [
            '[stage] parse',
            '[stage] resolve',
            '[stage] ir',
            '[stage] backend',
        ],
        'pipeline-stage-order',
    ))

    # Validation contracts: AST validate before resolve, HIR validate before MIR/backend.
    if 'frontend::validate::validate_module(ast_ctx, ast, diagnostics);' not in pipeline:
        errs.append('pipeline-validate: missing frontend AST validation call')
    if 'ir::validate::validate_module(hir_ctx, hir, diagnostics);' not in pipeline:
        errs.append('pipeline-validate: missing HIR validation call')

    # Passes contract: parse -> modules/rewrite -> validate -> resolve -> lower/validate.
    errs.extend(require_sequence(
        passes,
        [
            'auto module = parser.parse_module();',
            'frontend::modules::load_modules(',
            'frontend::modules::rewrite_member_access(',
            'frontend::validate::validate_module(ast_ctx, module, diagnostics);',
            'frontend::resolve::Resolver resolver(',
            'resolver.resolve_module(ast_ctx, module);',
            'frontend::lower::lower_to_hir(',
            'ir::validate::validate_module(hir_ctx, hir, diagnostics);',
        ],
        'passes-order',
    ))

    # Determinism surface must remain exposed.
    if '--deterministic' not in (ROOT / 'src/compiler/driver/options.cpp').read_text(encoding='utf-8'):
        errs.append('options: --deterministic flag missing')

    # Strict recovery must remain wired in both passes and pipeline.
    if '--strict-recovery=' not in passes:
        errs.append('passes: strict-recovery check missing')
    if '--strict-recovery=' not in pipeline:
        errs.append('pipeline: strict-recovery check missing')

    if errs:
        print('[pass-contract-audit] FAILED')
        for e in errs:
            print(f'- {e}')
        return 1

    print('[pass-contract-audit] OK')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
