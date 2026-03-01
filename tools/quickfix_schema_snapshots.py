#!/usr/bin/env python3
from __future__ import annotations
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CFG = ROOT / 'tools/facade_packages.json'


def write_schema(pkg: str, version: str, required_meta: bool) -> None:
    cdir = ROOT / f'tests/modules/contracts/{pkg}'
    cdir.mkdir(parents=True, exist_ok=True)
    suffix = 'v2' if required_meta else 'v1'
    out = cdir / f'{pkg}.quickfix.schema.{suffix}.json'
    payload = {
        'schema_version': version,
        'package': f'vitte/{pkg}',
        'commands': [
            'quickfix_preview',
            'quickfix_apply'
        ],
        'required_keys': ['code', 'payload'] + (['meta'] if required_meta else [])
    }
    out.write_text(json.dumps(payload, indent=2, sort_keys=True) + '\n', encoding='utf-8')


def main() -> int:
    cfg = json.loads(CFG.read_text(encoding='utf-8'))
    for ent in cfg['packages']:
        p = ent['name']
        write_schema(p, 'v1', False)
        write_schema(p, 'v2', True)
    print('[quickfix-schema] generated package quickfix schemas v1/v2')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
