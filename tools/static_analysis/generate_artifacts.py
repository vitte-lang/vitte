#!/usr/bin/env python3
from __future__ import annotations

import csv
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'target' / 'static_analysis'
REP = ROOT / 'target' / 'reports' / 'static_analysis_coverage.md'
JSON_OUT = OUT / 'analysis.json'
CSV_OUT = OUT / 'fixture_metrics.csv'
FIXTURES = ROOT / 'tests' / 'analysis'

METRIC_RE = re.compile(r"metrics:\s*cfg_blocks=(\d+)\s+alias_classes=(\d+)\s+points_to_sets=(\d+)")

TARGETS = {
    'cfg_blocks_total': 24,
    'alias_classes_total': 14,
    'points_to_sets_total': 19,
    'fixtures_count': 5,
}


def parse_fixture(path: Path):
    txt = path.read_text(encoding='utf-8')
    m = METRIC_RE.search(txt)
    if not m:
        raise RuntimeError(f'missing metrics tag in {path}')
    return {
        'file': str(path.relative_to(ROOT)),
        'cfg_blocks': int(m.group(1)),
        'alias_classes': int(m.group(2)),
        'points_to_sets': int(m.group(3)),
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
        'cfg_blocks_total': sum(x['cfg_blocks'] for x in fixture_metrics),
        'alias_classes_total': sum(x['alias_classes'] for x in fixture_metrics),
        'points_to_sets_total': sum(x['points_to_sets'] for x in fixture_metrics),
    }

    passes = {
        'data_flow_analysis': pass_fail(totals['fixtures_count'], TARGETS['fixtures_count']),
        'control_flow_graphs': pass_fail(totals['cfg_blocks_total'], TARGETS['cfg_blocks_total']),
        'alias_analysis': pass_fail(totals['alias_classes_total'], TARGETS['alias_classes_total']),
        'points_to_analysis': pass_fail(totals['points_to_sets_total'], TARGETS['points_to_sets_total']),
    }

    analyses_txt = OUT / 'analyses.txt'
    analyses_txt.write_text(
        '\n'.join([
            f"data_flow_analysis:{passes['data_flow_analysis']} ({totals['fixtures_count']}/{TARGETS['fixtures_count']})",
            f"control_flow_graphs:{passes['control_flow_graphs']} ({totals['cfg_blocks_total']}/{TARGETS['cfg_blocks_total']})",
            f"alias_analysis:{passes['alias_analysis']} ({totals['alias_classes_total']}/{TARGETS['alias_classes_total']})",
            f"points_to_analysis:{passes['points_to_analysis']} ({totals['points_to_sets_total']}/{TARGETS['points_to_sets_total']})",
        ])
        + '\n',
        encoding='utf-8',
    )

    with CSV_OUT.open('w', encoding='utf-8', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=['file', 'cfg_blocks', 'alias_classes', 'points_to_sets'])
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
        '# Static Analysis Coverage\n\n'
        f"- Data flow analysis: {passes['data_flow_analysis']} ({totals['fixtures_count']}/{TARGETS['fixtures_count']})\n"
        f"- Control flow graphs: {passes['control_flow_graphs']} ({totals['cfg_blocks_total']}/{TARGETS['cfg_blocks_total']})\n"
        f"- Alias analysis: {passes['alias_analysis']} ({totals['alias_classes_total']}/{TARGETS['alias_classes_total']})\n"
        f"- Points-to analysis: {passes['points_to_analysis']} ({totals['points_to_sets_total']}/{TARGETS['points_to_sets_total']})\n\n"
        '## Quantitative Metrics\n'
        f"- Fixtures analyzed: {totals['fixtures_count']}\n"
        f"- CFG blocks (total): {totals['cfg_blocks_total']}\n"
        f"- Alias classes (total): {totals['alias_classes_total']}\n"
        f"- Points-to sets (total): {totals['points_to_sets_total']}\n\n"
        '## Machine Exports\n'
        '- target/static_analysis/analysis.json\n'
        '- target/static_analysis/fixture_metrics.csv\n',
        encoding='utf-8',
    )

    print('[static-analysis] artifacts generated')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
