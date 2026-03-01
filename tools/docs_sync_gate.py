#!/usr/bin/env python3
from __future__ import annotations
import argparse
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def changed_files(base_ref: str) -> list[str]:
    out = subprocess.check_output(['git', '-C', str(ROOT), 'diff', '--name-only', f'{base_ref}...HEAD'], text=True)
    return [l.strip() for l in out.splitlines() if l.strip()]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument('--base-ref', default='HEAD~1')
    args = ap.parse_args()

    try:
        files = changed_files(args.base_ref)
    except Exception:
        print('[docs-sync][warn] git diff unavailable; skip')
        return 0

    changed = set(files)
    errs = []
    for f in files:
        parts = f.split('/')
        if len(parts) == 5 and parts[0] == 'src' and parts[1] == 'vitte' and parts[2] == 'packages' and parts[4] == 'mod.vit':
            pkg = parts[3]
            doc = f'docs/{pkg}/API_INDEX.md'
            if doc not in changed:
                errs.append(f'{pkg}: facade changed without docs sync ({doc})')

    if errs:
        for e in errs:
            print(f'[docs-sync][error] {e}')
        return 1
    print('[docs-sync] OK')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
