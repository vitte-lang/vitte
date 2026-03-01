#!/usr/bin/env python3
from pathlib import Path

def main()->int:
    repo=Path(__file__).resolve().parents[1]
    txt=(repo/'src/vitte/packages/json/mod.vit').read_text(encoding='utf-8')
    itxt=(repo/'src/vitte/packages/json/internal/runtime.vit').read_text(encoding='utf-8')
    errs=[]
    for need in ['max_bytes','max_depth','max_nodes','strict_utf8','strict_escapes','parse_stream','write_stream']:
        if need not in txt: errs.append(f'missing {need} in facade')
    for need in ['VITTE-J0004','VITTE-J0011','validate_parse_input','quickfix_for']:
        if need not in itxt: errs.append(f'missing {need} in runtime')
    if errs:
        for e in errs: print(f'[json-security-lint][error] {e}')
        return 1
    print('[json-security-lint] OK'); return 0
if __name__=='__main__': raise SystemExit(main())
