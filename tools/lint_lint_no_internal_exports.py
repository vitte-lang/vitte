#!/usr/bin/env python3
from pathlib import Path

def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    p = repo / 'tests/modules/contracts/lint/lint.exports.internal'
    if not p.exists():
        print(f'[lint-no-internal-exports][error] missing snapshot: {p}')
        return 1
    if p.read_text(encoding='utf-8').strip():
        print(f'[lint-no-internal-exports][error] {p}: must stay empty')
        return 1
    print('[lint-no-internal-exports] OK')
    return 0
if __name__ == '__main__':
    raise SystemExit(main())
