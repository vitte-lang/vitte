#!/usr/bin/env python3
from pathlib import Path

def check(root: Path, tag: str)->int:
    errs=[]
    for p in root.rglob('*.vit'):
        for i,l in enumerate(p.read_text(encoding='utf-8').splitlines(),1):
            if l.strip().startswith('entry '): errs.append(f'{p}:{i}: entry forbidden')
    if errs:
        for e in errs: print(f'[{tag}][error] {e}')
        return 1
    print(f'[{tag}] OK'); return 0

if __name__=='__main__':
    repo=Path(__file__).resolve().parents[1]
    a=check(repo/'src/vitte/packages/http','http-no-side-effects')
    b=check(repo/'src/vitte/packages/http_client','http-client-no-side-effects')
    raise SystemExit(1 if (a or b) else 0)
