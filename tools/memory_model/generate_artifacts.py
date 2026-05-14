#!/usr/bin/env python3
from __future__ import annotations

import csv
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'target' / 'memory_model'
REP = ROOT / 'target' / 'reports' / 'memory_model_coverage.md'
JSON_OUT = OUT / 'analysis.json'
CSV_OUT = OUT / 'fixture_metrics.csv'
FIXTURES = ROOT / 'tests' / 'memory_model'

METRIC_RE = re.compile(
    r"metrics:\s*gc_cycles=(\d+)\s+manual_memory_ops=(\d+)\s+ownership_checks=(\d+)\s+zero_cost_paths=(\d+)\s+simd_kernels=(\d+)"
)

TARGETS = {
    'fixtures_count': 5,
    'gc_cycles_total': 10,
    'manual_memory_ops_total': 25,
    'ownership_checks_total': 20,
    'zero_cost_paths_total': 18,
    'simd_kernels_total': 15,
}


def parse_fixture(path: Path):
    txt = path.read_text(encoding='utf-8')
    m = METRIC_RE.search(txt)
    if not m:
        raise RuntimeError(f'missing metrics tag in {path}')
    return {
        'file': str(path.relative_to(ROOT)),
        'gc_cycles': int(m.group(1)),
        'manual_memory_ops': int(m.group(2)),
        'ownership_checks': int(m.group(3)),
        'zero_cost_paths': int(m.group(4)),
        'simd_kernels': int(m.group(5)),
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
        'gc_cycles_total': sum(x['gc_cycles'] for x in fixture_metrics),
        'manual_memory_ops_total': sum(x['manual_memory_ops'] for x in fixture_metrics),
        'ownership_checks_total': sum(x['ownership_checks'] for x in fixture_metrics),
        'zero_cost_paths_total': sum(x['zero_cost_paths'] for x in fixture_metrics),
        'simd_kernels_total': sum(x['simd_kernels'] for x in fixture_metrics),
    }

    passes = {
        'garbage_collection_optional': pass_fail(totals['gc_cycles_total'], TARGETS['gc_cycles_total']),
        'manual_memory_management': pass_fail(totals['manual_memory_ops_total'], TARGETS['manual_memory_ops_total']),
        'ownership_system_rust_like': pass_fail(totals['ownership_checks_total'], TARGETS['ownership_checks_total']),
        'zero_cost_abstractions': pass_fail(totals['zero_cost_paths_total'], TARGETS['zero_cost_paths_total']),
        'simd_intrinsics': pass_fail(totals['simd_kernels_total'], TARGETS['simd_kernels_total']),
    }

    (OUT / 'features.txt').write_text(
        '\n'.join([
            f"garbage_collection_optional:{passes['garbage_collection_optional']} ({totals['gc_cycles_total']}/{TARGETS['gc_cycles_total']})",
            f"manual_memory_management:{passes['manual_memory_management']} ({totals['manual_memory_ops_total']}/{TARGETS['manual_memory_ops_total']})",
            f"ownership_system_rust_like:{passes['ownership_system_rust_like']} ({totals['ownership_checks_total']}/{TARGETS['ownership_checks_total']})",
            f"zero_cost_abstractions:{passes['zero_cost_abstractions']} ({totals['zero_cost_paths_total']}/{TARGETS['zero_cost_paths_total']})",
            f"simd_intrinsics:{passes['simd_intrinsics']} ({totals['simd_kernels_total']}/{TARGETS['simd_kernels_total']})",
        ]) + '\n',
        encoding='utf-8',
    )

    with CSV_OUT.open('w', encoding='utf-8', newline='') as f:
        writer = csv.DictWriter(
            f,
            fieldnames=['file', 'gc_cycles', 'manual_memory_ops', 'ownership_checks', 'zero_cost_paths', 'simd_kernels'],
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
        '# Memory & Performance Coverage\n\n'
        f"- Garbage collection (optional): {passes['garbage_collection_optional']} ({totals['gc_cycles_total']}/{TARGETS['gc_cycles_total']})\n"
        f"- Manual memory management: {passes['manual_memory_management']} ({totals['manual_memory_ops_total']}/{TARGETS['manual_memory_ops_total']})\n"
        f"- Ownership system (Rust-like): {passes['ownership_system_rust_like']} ({totals['ownership_checks_total']}/{TARGETS['ownership_checks_total']})\n"
        f"- Zero-cost abstractions: {passes['zero_cost_abstractions']} ({totals['zero_cost_paths_total']}/{TARGETS['zero_cost_paths_total']})\n"
        f"- SIMD intrinsics: {passes['simd_intrinsics']} ({totals['simd_kernels_total']}/{TARGETS['simd_kernels_total']})\n\n"
        '## Quantitative Metrics\n'
        f"- Fixtures analyzed: {totals['fixtures_count']}\n"
        f"- GC cycles modeled (total): {totals['gc_cycles_total']}\n"
        f"- Manual memory ops (total): {totals['manual_memory_ops_total']}\n"
        f"- Ownership checks (total): {totals['ownership_checks_total']}\n"
        f"- Zero-cost paths (total): {totals['zero_cost_paths_total']}\n"
        f"- SIMD kernels (total): {totals['simd_kernels_total']}\n\n"
        '## Machine Exports\n'
        '- target/memory_model/analysis.json\n'
        '- target/memory_model/fixture_metrics.csv\n',
        encoding='utf-8',
    )

    print('[memory-model] artifacts generated')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
