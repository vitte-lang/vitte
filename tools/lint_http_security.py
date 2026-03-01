#!/usr/bin/env python3
from pathlib import Path

def main()->int:
    repo=Path(__file__).resolve().parents[1]
    http_txt=(repo/'src/vitte/packages/http/mod.vit').read_text(encoding='utf-8')
    cli_txt=(repo/'src/vitte/packages/http_client/mod.vit').read_text(encoding='utf-8')
    errs=[]
    for need in ['apply_security_gates','VITTE-H0002','VITTE-H0009','with_cors','with_csrf','with_rate_limit']:
        if need not in http_txt: errs.append(f'http missing {need}')
    for need in ['validate_request','VITTE-C0002','VITTE-C0003','security_redirect_allowed','strict_tls']:
        if need not in cli_txt: errs.append(f'http_client missing {need}')
    if errs:
        for e in errs: print(f'[http-security-lint][error] {e}')
        return 1
    print('[http-security-lint] OK'); return 0
if __name__=='__main__': raise SystemExit(main())
