#!/usr/bin/env python3
from __future__ import annotations
import re
from pathlib import Path
DIAG_PREFIX='VITTE-H'

def main() -> int:
    repo=Path(__file__).resolve().parents[1]
    mod=repo/'src/vitte/packages/http/mod.vit'
    txt=mod.read_text(encoding='utf-8') if mod.exists() else ''
    errs=[]
    if 'PREAMBLE (API stable facade)' not in txt: errs.append('missing PREAMBLE')
    if '<<< ROLE-CONTRACT' not in txt: errs.append('missing ROLE-CONTRACT')
    if 'use vitte/http/internal/' not in txt: errs.append('missing internal bridge')
    for k in ('versioning:','api_surface_stable:','diagnostics:'): 
        if k not in txt: errs.append(f'missing {k}')
    if DIAG_PREFIX not in txt: errs.append('missing diagnostics prefix')
    if 'entry ' in txt: errs.append('entry forbidden')
    name_re=re.compile(r'^([a-z][a-z0-9_]*|[A-Z][A-Za-z0-9]*)$')
    for ln in txt.splitlines():
        m=re.match(r'\s*(pick|form|proc)\s+([A-Za-z_][A-Za-z0-9_]*)',ln)
        if m and not name_re.match(m.group(2)): errs.append(f'invalid export name {m.group(2)}')
    if errs:
        for e in errs: print(f'[http-mod-lint][error] {e}')
        return 1
    print('[http-mod-lint] OK'); return 0
if __name__=='__main__': raise SystemExit(main())
