#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MOD = ROOT / 'src' / 'vitte' / 'compiler' / 'types' / 'advanced' / 'mod.vit'
SMOKE = ROOT / 'src' / 'vitte' / 'compiler' / 'types' / 'advanced' / 'tests' / 'smoke.vit'
FIXTURES = ROOT / 'tests' / 'type_system'

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

REQUIRED_SYMBOLS = [
    'generic_catalog',
    'trait_catalog',
    'sum_catalog',
    'pattern_catalog',
    'inference_catalog',
    'compute_metrics',
    'generics_templates',
    'traits_interfaces',
    'sum_types',
    'advanced_pattern_matching',
    'type_inference_complete',
    'run_all_type_features',
]

REQUIRED_SMOKE_SYMBOLS = [
    'smoke_type_feature_count',
    'smoke_type_feature_success',
    'smoke_type_metrics_thresholds',
]


def fail(msg: str) -> int:
    print(f'[type-system][error] {msg}', file=sys.stderr)
    return 1


def main() -> int:
    if not MOD.exists() or not SMOKE.exists():
        return fail('missing type system module or smoke test')
    if not FIXTURES.exists():
        return fail('missing tests/type_system fixtures directory')

    present = {p.name for p in FIXTURES.glob('*.vit')}
    missing = sorted(REQUIRED_FIXTURES - present)
    if missing:
        return fail(f'missing fixtures: {", ".join(missing)}')

    mod_text = MOD.read_text(encoding='utf-8')
    for sym in REQUIRED_SYMBOLS:
        if sym not in mod_text:
            return fail(f'missing symbol in mod.vit: {sym}')

    smoke_text = SMOKE.read_text(encoding='utf-8')
    for sym in REQUIRED_SMOKE_SYMBOLS:
        if sym not in smoke_text:
            return fail(f'missing symbol in smoke.vit: {sym}')

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
