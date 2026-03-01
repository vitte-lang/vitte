#!/usr/bin/env python3
from __future__ import annotations
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPORTS = ROOT / 'target/reports'
OUT = REPORTS / 'index.json'


def try_json(path: Path):
    try:
        return json.loads(path.read_text(encoding='utf-8'))
    except Exception:
        return None


def main() -> int:
    REPORTS.mkdir(parents=True, exist_ok=True)
    data = {
        'schema_version': '1.0',
        'reports': [],
        'aliases': {}
    }
    for p in sorted(REPORTS.glob('*')):
        if not p.is_file() or p.name == 'index.json':
            continue
        item = {
            'name': p.name,
            'path': str(p.relative_to(ROOT)),
            'size': p.stat().st_size,
            'kind': p.suffix.lstrip('.') or 'txt'
        }
        j = try_json(p)
        if isinstance(j, dict) and 'schema_version' in j:
            item['schema_version'] = j['schema_version']
        data['reports'].append(item)

    # common aliases for IDE/pipeline consumers
    for key in ['contracts_dashboard.md', 'diagnostics_index.json', 'security_gates.report', 'release_doctor.json', 'release_doctor.md', 'repro.json']:
        if (REPORTS / key).exists():
            data['aliases'][key] = str((REPORTS / key).relative_to(ROOT))

    OUT.write_text(json.dumps(data, indent=2, sort_keys=True) + '\n', encoding='utf-8')
    print(f'[reports-index] wrote {OUT}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
