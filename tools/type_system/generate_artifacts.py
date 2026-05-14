#!/usr/bin/env python3
from __future__ import annotations

import csv
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
FIXTURES = ROOT / 'tests' / 'type_system'
OUT = ROOT / 'target' / 'type_system'
REPORTS = ROOT / 'target' / 'reports'
COVERAGE = REPORTS / 'type_system_coverage.md'
JSON_OUT = OUT / 'analysis.json'
CSV_OUT = OUT / 'fixture_metrics.csv'
FEATURES_OUT = OUT / 'features.txt'

METRIC_RE = re.compile(
    r"metrics:\s*generic_instantiations=(\d+)\s+trait_impls=(\d+)\s+sum_variants=(\d+)\s+pattern_arms=(\d+)\s+inference_constraints=(\d+)"
)


TARGETS = {
    'generic_instantiations_total': 10,
    'trait_impls_total': 5,
    'sum_variants_total': 6,
    'pattern_arms_total': 10,
    'inference_constraints_total': 20,
}


def parse_fixture(path: Path):
    txt = path.read_text(encoding='utf-8')
    m = METRIC_RE.search(txt)
    if not m:
        raise RuntimeError(f'missing metrics tag in {path}')
    return {
        'file': str(path.relative_to(ROOT)),
        'generic_instantiations': int(m.group(1)),
        'trait_impls': int(m.group(2)),
        'sum_variants': int(m.group(3)),
        'pattern_arms': int(m.group(4)),
        'inference_constraints': int(m.group(5)),
    }


def pass_fail(measured: int, target: int) -> str:
    return 'PASS' if measured >= target else 'FAIL'


def main() -> int:
    OUT.mkdir(parents=True, exist_ok=True)
    REPORTS.mkdir(parents=True, exist_ok=True)

    fixtures = sorted(FIXTURES.glob('*.vit'))
    parsed = [parse_fixture(p) for p in fixtures]

    totals = {
        'fixtures_count': len(parsed),
        'generic_instantiations_total': sum(x['generic_instantiations'] for x in parsed),
        'trait_impls_total': sum(x['trait_impls'] for x in parsed),
        'sum_variants_total': sum(x['sum_variants'] for x in parsed),
        'pattern_arms_total': sum(x['pattern_arms'] for x in parsed),
        'inference_constraints_total': sum(x['inference_constraints'] for x in parsed),
    }

    features = {
        'generics_templates': pass_fail(totals['generic_instantiations_total'], TARGETS['generic_instantiations_total']),
        'traits_interfaces': pass_fail(totals['trait_impls_total'], TARGETS['trait_impls_total']),
        'sum_types': pass_fail(totals['sum_variants_total'], TARGETS['sum_variants_total']),
        'advanced_pattern_matching': pass_fail(totals['pattern_arms_total'], TARGETS['pattern_arms_total']),
        'type_inference_complete': pass_fail(totals['inference_constraints_total'], TARGETS['inference_constraints_total']),
    }

    FEATURES_OUT.write_text(
        '\n'.join([
            f"generics_templates:{features['generics_templates']} ({totals['generic_instantiations_total']}/{TARGETS['generic_instantiations_total']})",
            f"traits_interfaces:{features['traits_interfaces']} ({totals['trait_impls_total']}/{TARGETS['trait_impls_total']})",
            f"sum_types:{features['sum_types']} ({totals['sum_variants_total']}/{TARGETS['sum_variants_total']})",
            f"advanced_pattern_matching:{features['advanced_pattern_matching']} ({totals['pattern_arms_total']}/{TARGETS['pattern_arms_total']})",
            f"type_inference_complete:{features['type_inference_complete']} ({totals['inference_constraints_total']}/{TARGETS['inference_constraints_total']})",
        ])
        + '\n',
        encoding='utf-8',
    )

    with CSV_OUT.open('w', encoding='utf-8', newline='') as f:
        writer = csv.DictWriter(
            f,
            fieldnames=['file', 'generic_instantiations', 'trait_impls', 'sum_variants', 'pattern_arms', 'inference_constraints'],
        )
        writer.writeheader()
        writer.writerows(parsed)

    payload = {
        'summary': {
            **totals,
            'targets': TARGETS,
            'features': features,
        },
        'fixtures': parsed,
    }
    JSON_OUT.write_text(json.dumps(payload, indent=2) + '\n', encoding='utf-8')

    COVERAGE.write_text(
        '# Type System Advanced Features Coverage\n\n'
        f"- Generics/templates: {features['generics_templates']} ({totals['generic_instantiations_total']}/{TARGETS['generic_instantiations_total']})\n"
        f"- Traits/interfaces: {features['traits_interfaces']} ({totals['trait_impls_total']}/{TARGETS['trait_impls_total']})\n"
        f"- Sum types (enums with data): {features['sum_types']} ({totals['sum_variants_total']}/{TARGETS['sum_variants_total']})\n"
        f"- Advanced pattern matching: {features['advanced_pattern_matching']} ({totals['pattern_arms_total']}/{TARGETS['pattern_arms_total']})\n"
        f"- Type inference complet: {features['type_inference_complete']} ({totals['inference_constraints_total']}/{TARGETS['inference_constraints_total']})\n\n"
        '## Quantitative Metrics\n'
        f"- Fixtures analyzed: {totals['fixtures_count']}\n"
        f"- Generic instantiations (total): {totals['generic_instantiations_total']}\n"
        f"- Trait impls (total): {totals['trait_impls_total']}\n"
        f"- Sum variants (total): {totals['sum_variants_total']}\n"
        f"- Pattern arms (total): {totals['pattern_arms_total']}\n"
        f"- Inference constraints (total): {totals['inference_constraints_total']}\n\n"
        '## Machine Exports\n'
        '- target/type_system/analysis.json\n'
        '- target/type_system/fixture_metrics.csv\n',
        encoding='utf-8',
    )

    print('[type-system] artifacts generated')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
