#!/usr/bin/env python3
from __future__ import annotations
from pathlib import Path

def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    p = repo / 'tests/modules/contracts/yaml/yaml.exports.internal'
    if not p.exists():
        print(f'[yaml-no-internal-exports][error] missing snapshot: {p}')
        return 1
    if p.read_text(encoding='utf-8').strip():
        print(f'[yaml-no-internal-exports][error] {p}: must stay empty')
        return 1
    print('[yaml-no-internal-exports] OK')
    return 0
if __name__ == '__main__':
    raise SystemExit(main())
