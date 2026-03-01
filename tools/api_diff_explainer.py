#!/usr/bin/env python3
from __future__ import annotations
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CFG = ROOT / 'tools/facade_packages.json'
OUT = ROOT / 'target/reports/api_diff_explainer.md'


def read_set(p: Path) -> set[str]:
    if not p.exists():
        return set()
    return {l.strip() for l in p.read_text(encoding='utf-8').splitlines() if l.strip()}


def main() -> int:
    cfg = json.loads(CFG.read_text(encoding='utf-8'))
    lines = ['# API Diff Explainer', '']
    has_breaking = False

    for ent in cfg['packages']:
        pkg = ent['name']
        cdir = ROOT / f'tests/modules/contracts/{pkg}'
        baseline = read_set(cdir / f'{pkg}.facade.api')
        current = read_set(cdir / f'{pkg}.exports')
        removed = sorted(baseline - current)
        added = sorted(current - baseline)
        lines.append(f'## {pkg}')
        if not removed and not added:
            lines.append('- Status: no API diff')
            lines.append('')
            continue
        if removed:
            has_breaking = True
            lines.append('- Status: BREAKING (symbol removals detected)')
            lines.append('- Why breaking: additive-only policy forbids removals in stable line')
            lines.append('- Impact: downstream imports/IDE quickfix mappings may fail')
            lines.append('- Migration hints:')
            lines.append('  - Restore removed symbol as compatibility wrapper')
            lines.append('  - Or bump major and provide migration note')
            lines.append('  - Or alias old symbol to new stable symbol')
            lines.append('- Removed symbols:')
            for s in removed[:40]:
                lines.append(f'  - `{s}`')
        if added:
            lines.append('- Added symbols:')
            for s in added[:40]:
                lines.append(f'  - `{s}`')
        lines.append('')

    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text('\n'.join(lines) + '\n', encoding='utf-8')
    print(f'[api-diff-explainer] wrote {OUT}')
    if has_breaking:
        print('[api-diff-explainer] BREAKING entries present; see report')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
