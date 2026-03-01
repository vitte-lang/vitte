#!/usr/bin/env python3
import re
from pathlib import Path
USE=re.compile(r'^\s*use\s+([^\s]+)\s+as\s+([A-Za-z_][A-Za-z0-9_]*)')

def main() -> int:
    repo=Path(__file__).resolve().parents[1]
    roots=[repo/'src/vitte/packages/process', repo/'tests/process', repo/'tests/modules/contracts/process']
    errs=[]
    for root in roots:
        if not root.exists():
            continue
        for p in root.rglob('*.vit'):
            for i,l in enumerate(p.read_text(encoding='utf-8').splitlines(),1):
                m=USE.match(l)
                if m and not m.group(2).endswith('_pkg'):
                    errs.append(f'{p}:{i}: alias must end with _pkg')
    if errs:
        for e in errs:
            print(f'[process-alias-pkg-lint][error] {e}')
        return 1
    print('[process-alias-pkg-lint] OK')
    return 0
if __name__=='__main__':
    raise SystemExit(main())
