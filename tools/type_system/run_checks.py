#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
FIXTURES = ROOT / 'tests' / 'type_system'

CONTRACT_FILES = {
    'typeck_mod': ROOT / 'src' / 'vitte' / 'compiler' / 'analysis' / 'typeck' / 'mod.vit',
    'typeck_infer': ROOT / 'src' / 'vitte' / 'compiler' / 'analysis' / 'typeck' / 'infer.vit',
    'typeck_traits': ROOT / 'src' / 'vitte' / 'compiler' / 'analysis' / 'typeck' / 'traits.vit',
    'typeck_unify': ROOT / 'src' / 'vitte' / 'compiler' / 'analysis' / 'typeck' / 'unify.vit',
    'typeck_coercion': ROOT / 'src' / 'vitte' / 'compiler' / 'analysis' / 'typeck' / 'coercion.vit',
    'typeck_tests': ROOT / 'src' / 'vitte' / 'compiler' / 'tests' / 'typeck_tests.vit',
}

METRIC_RE = re.compile(
    r"metrics:\s*generic_instantiations=(\d+)\s+trait_impls=(\d+)\s+sum_variants=(\d+)\s+pattern_arms=(\d+)\s+inference_constraints=(\d+)"
)

REQUIRED_FIXTURES = {
    'generics_positive.vit',
    'traits_positive.vit',
    'sum_pattern_positive.vit',
    'inference_positive.vit',
    'edge_ambiguous_constraints.vit',
}

REQUIRED_SYMBOLS = {
    'typeck_mod': [
        'checker',
        'coercion',
        'infer',
        'traits',
        'unify',
        'soundness_roadmap',
    ],
    'typeck_infer': [
        'TypeInferContext',
        'infer_call_type',
        'type_binding',
        'type_env',
        'type_is_copy',
        'unify_types',
    ],
    'typeck_traits': [
        'TypeClass',
        'type_is_numeric',
        'type_is_truthy',
        'type_is_known',
        'type_is_function',
        'normalize_type_name',
    ],
    'typeck_unify': [
        'TypeUnifyResult',
        'unify_types',
        'unify_binary_result_type',
        'unify_assignment_type',
        'NumericPromotion',
    ],
    'typeck_coercion': [
        'TypeProjectionResult',
        'type_deref_target',
        'type_index_element',
        'types_compatible',
        'projection_member_name',
    ],
    'typeck_tests': [
        'run_production_typeck_hir',
        'infer_call_type',
        'test_valid_numeric_flow',
        'test_assign_mismatch',
        'test_return_mismatch_carries_rich_diagnostic_context',
        'test_call_arity_carries_signature_context',
    ],
}


def fail(msg: str) -> int:
    print(f'[type-system][error] {msg}', file=sys.stderr)
    return 1


def main() -> int:
    missing_contract_files = [name for name, path in CONTRACT_FILES.items() if not path.exists()]
    if missing_contract_files:
        return fail(f'missing type system contract files: {", ".join(sorted(missing_contract_files))}')
    if not FIXTURES.exists():
        return fail('missing tests/type_system fixtures directory')

    present = {p.name for p in FIXTURES.glob('*.vit')}
    missing = sorted(REQUIRED_FIXTURES - present)
    if missing:
        return fail(f'missing fixtures: {", ".join(missing)}')

    for name, path in CONTRACT_FILES.items():
        text = path.read_text(encoding='utf-8')
        for sym in REQUIRED_SYMBOLS[name]:
            if sym not in text:
                return fail(f'missing symbol in {path.relative_to(ROOT)}: {sym}')

    fixture_files = sorted(FIXTURES.glob('*.vit'))
    if len(fixture_files) < 5:
        return fail('insufficient fixture count (<5)')

    for fixture in fixture_files:
        txt = fixture.read_text(encoding='utf-8')
        m = METRIC_RE.search(txt)
        if not m:
            return fail(f'missing metrics tag in {fixture.relative_to(ROOT)}')
        vals = [int(m.group(i)) for i in range(1, 6)]
        if any(v < 0 for v in vals):
            return fail(f'negative metrics in {fixture.relative_to(ROOT)}')

    print('[type-system] checks passed')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
