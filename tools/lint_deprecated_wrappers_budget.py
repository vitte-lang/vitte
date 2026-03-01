#!/usr/bin/env python3
from __future__ import annotations
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    errs=[]
    for p in (ROOT/'src/vitte/packages').rglob('mod.vit'):
        txt=p.read_text(encoding='utf-8')
        for m in re.finditer(r'DEPRECATED[^\n]*removal target:\s*v([0-9]+)\.([0-9]+)\.([0-9]+)', txt):
            major=int(m.group(1))
            if major <= 3:
                errs.append(f'{p}: removal target too close/expired: {m.group(0)}')
    if errs:
        for e in errs:
            print(f'[deprecated-budget][error] {e}')
        return 1
    print('[deprecated-budget] OK')
    return 0

if __name__=='__main__':
    raise SystemExit(main())
