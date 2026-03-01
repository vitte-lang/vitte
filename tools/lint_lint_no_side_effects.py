#!/usr/bin/env python3
from pathlib import Path

def main() -> int:
    root = Path(__file__).resolve().parents[1] / 'src/vitte/packages/lint'
    errs=[]
    for p in root.rglob('*.vit'):
        for i,l in enumerate(p.read_text(encoding='utf-8').splitlines(),1):
            if l.strip().startswith('entry '):
                errs.append(f'{p}:{i}: entry forbidden')
    if errs:
        for e in errs:
            print(f'[lint-no-side-effects][error] {e}')
        return 1
    print('[lint-no-side-effects] OK')
    return 0
if __name__=='__main__':
    raise SystemExit(main())
