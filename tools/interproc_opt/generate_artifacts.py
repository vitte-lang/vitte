#!/usr/bin/env python3
from __future__ import annotations

import csv
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'target' / 'interproc_opt'
REP = ROOT / 'target' / 'reports' / 'interproc_opt_coverage.md'
JSON_OUT = OUT / 'analysis.json'
CSV_OUT = OUT / 'fixture_metrics.csv'
FIXTURES = ROOT / 'tests' / 'interproc'

METRIC_RE = re.compile(
    r"metrics:\s*devirt_sites=(\d+)\s+specialized_functions=(\d+)\s+cross_module_rewrites=(\d+)\s+lto_internalized=(\d+)"
)

TARGETS = {
    'fixtures_count': 5,
    'devirt_sites_total': 15,
    'specialized_functions_total': 12,
    'cross_module_rewrites_total': 15,
    'lto_internalized_total': 12,
}


def parse_fixture(path: Path):
    txt = path.read_text(encoding='utf-8')
    m = METRIC_RE.search(txt)
    if not m:
        raise RuntimeError(f'missing metrics tag in {path}')
    return {
        'file': str(path.relative_to(ROOT)),
        'devirt_sites': int(m.group(1)),
        'specialized_functions': int(m.group(2)),
        'cross_module_rewrites': int(m.group(3)),
        'lto_internalized': int(m.group(4)),
    }


def pass_fail(measured: int, target: int) -> str:
    return 'PASS' if measured >= target else 'FAIL'


def main() -> int:
    OUT.mkdir(parents=True, exist_ok=True)
    (ROOT / 'target' / 'reports').mkdir(parents=True, exist_ok=True)

    fixture_files = sorted(FIXTURES.glob('*.vit'))
    fixture_metrics = [parse_fixture(p) for p in fixture_files]

    totals = {
        'fixtures_count': len(fixture_metrics),
        'devirt_sites_total': sum(x['devirt_sites'] for x in fixture_metrics),
        'specialized_functions_total': sum(x['specialized_functions'] for x in fixture_metrics),
        'cross_module_rewrites_total': sum(x['cross_module_rewrites'] for x in fixture_metrics),
        'lto_internalized_total': sum(x['lto_internalized'] for x in fixture_metrics),
    }

    passes = {
        'devirtualization': pass_fail(totals['devirt_sites_total'], TARGETS['devirt_sites_total']),
        'function_specialization': pass_fail(totals['specialized_functions_total'], TARGETS['specialized_functions_total']),
        'whole_program_optimization': pass_fail(totals['cross_module_rewrites_total'], TARGETS['cross_module_rewrites_total']),
        'link_time_optimization': pass_fail(totals['lto_internalized_total'], TARGETS['lto_internalized_total']),
    }

    passes_txt = OUT / 'passes.txt'
    passes_txt.write_text(
        '\n'.join([
            f"devirtualization:{passes['devirtualization']} ({totals['devirt_sites_total']}/{TARGETS['devirt_sites_total']})",
            f"function_specialization:{passes['function_specialization']} ({totals['specialized_functions_total']}/{TARGETS['specialized_functions_total']})",
            f"whole_program_optimization:{passes['whole_program_optimization']} ({totals['cross_module_rewrites_total']}/{TARGETS['cross_module_rewrites_total']})",
            f"link_time_optimization:{passes['link_time_optimization']} ({totals['lto_internalized_total']}/{TARGETS['lto_internalized_total']})",
        ])
        + '\n',
        encoding='utf-8',
    )

    with CSV_OUT.open('w', encoding='utf-8', newline='') as f:
        writer = csv.DictWriter(
            f,
            fieldnames=['file', 'devirt_sites', 'specialized_functions', 'cross_module_rewrites', 'lto_internalized'],
        )
        writer.writeheader()
        writer.writerows(fixture_metrics)

    payload = {
        'summary': {
            **totals,
            'targets': TARGETS,
            'passes': passes,
        },
        'fixtures': fixture_metrics,
    }
    JSON_OUT.write_text(json.dumps(payload, indent=2) + '\n', encoding='utf-8')

    REP.write_text(
        '# Interproc Optimizations Coverage\n\n'
        f"- Devirtualization: {passes['devirtualization']} ({totals['devirt_sites_total']}/{TARGETS['devirt_sites_total']})\n"
        f"- Function specialization: {passes['function_specialization']} ({totals['specialized_functions_total']}/{TARGETS['specialized_functions_total']})\n"
        f"- Whole program optimization: {passes['whole_program_optimization']} ({totals['cross_module_rewrites_total']}/{TARGETS['cross_module_rewrites_total']})\n"
        f"- Link-time optimization: {passes['link_time_optimization']} ({totals['lto_internalized_total']}/{TARGETS['lto_internalized_total']})\n\n"
        '## Quantitative Metrics\n'
        f"- Fixtures analyzed: {totals['fixtures_count']}\n"
        f"- Devirtualized callsites (total): {totals['devirt_sites_total']}\n"
        f"- Specialized functions (total): {totals['specialized_functions_total']}\n"
        f"- Cross-module rewrites (total): {totals['cross_module_rewrites_total']}\n"
        f"- LTO internalized symbols (total): {totals['lto_internalized_total']}\n\n"
        '## Machine Exports\n'
        '- target/interproc_opt/analysis.json\n'
        '- target/interproc_opt/fixture_metrics.csv\n',
        encoding='utf-8',
    )

    print('[interproc-opt] artifacts generated')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
