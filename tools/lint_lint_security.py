#!/usr/bin/env python3
from pathlib import Path

def main()->int:
    repo=Path(__file__).resolve().parents[1]
    txt=(repo/'src/vitte/packages/lint/mod.vit').read_text(encoding='utf-8')
    itxt=(repo/'src/vitte/packages/lint/internal/engine.vit').read_text(encoding='utf-8')
    errs=[]
    for need in ['quickfix_preview','quickfix_apply','LintConfig','lint_workspace','policy_allows_ci_block']:
        if need not in txt: errs.append(f'missing {need} in facade')
    for need in ['VITTE-I0001','VITTE-I0002','diag_message','quickfix_for']:
        if need not in itxt: errs.append(f'missing {need} in runtime')
    if errs:
        for e in errs: print(f'[lint-security-lint][error] {e}')
        return 1
    print('[lint-security-lint] OK'); return 0
if __name__=='__main__': raise SystemExit(main())
