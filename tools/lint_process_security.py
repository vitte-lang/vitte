#!/usr/bin/env python3
from pathlib import Path

def main()->int:
    repo=Path(__file__).resolve().parents[1]
    txt=(repo/'src/vitte/packages/process/mod.vit').read_text(encoding='utf-8')
    rt=(repo/'src/vitte/packages/process/internal/runtime.vit').read_text(encoding='utf-8')
    errs=[]
    for n in ['validate_config','VITTE-P0011','VITTE-P0013','VITTE-P0014','allow_shell forbidden on core']:
        if n not in txt:
            errs.append(f'missing mod.{n}')
    for n in ['shell_exec_pattern','command_allowlisted','command_denylisted','env_valid','args_valid']:
        if n not in rt:
            errs.append(f'missing runtime.{n}')
    if errs:
        for e in errs: print(f'[process-security-lint][error] {e}')
        return 1
    print('[process-security-lint] OK'); return 0
if __name__=='__main__': raise SystemExit(main())
