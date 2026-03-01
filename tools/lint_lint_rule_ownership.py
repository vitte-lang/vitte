#!/usr/bin/env python3
from __future__ import annotations
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RULES = ROOT / 'src/vitte/packages/lint/internal/rules.vit'
META = ROOT / 'tools/lint_rule_ownership.json'


def parse_rule_ids(txt: str) -> list[str]:
    return re.findall(r'LintRule\("([^"]+)"', txt)


def main() -> int:
    if not RULES.exists() or not META.exists():
        print('[lint-rule-ownership][error] missing rules.vit or lint_rule_ownership.json')
        return 1
    ids = parse_rule_ids(RULES.read_text(encoding='utf-8'))
    meta = json.loads(META.read_text(encoding='utf-8'))
    errs = []
    for rid in ids:
        m = meta.get(rid)
        if not m:
            errs.append(f'missing ownership metadata for rule {rid}')
            continue
        for key in ['owner', 'rationale', 'false_positive_policy']:
            if not str(m.get(key, '')).strip():
                errs.append(f'rule {rid}: missing {key}')
    if errs:
        for e in errs:
            print(f'[lint-rule-ownership][error] {e}')
        return 1
    print('[lint-rule-ownership] OK')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
