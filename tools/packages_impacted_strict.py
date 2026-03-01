#!/usr/bin/env python3
from __future__ import annotations
import argparse
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEPS = ROOT / 'tools/package_deps.json'


def get_changed(base_ref: str) -> list[str]:
    try:
        out = subprocess.check_output(['git', '-C', str(ROOT), 'diff', '--name-only', f'{base_ref}...HEAD'], text=True)
    except Exception:
        return []
    return [l.strip() for l in out.splitlines() if l.strip()]


def reverse_closure(pkgs: set[str], deps: dict[str, list[str]]) -> set[str]:
    changed = set(pkgs)
    while True:
        added = set()
        for p, pdeps in deps.items():
            if p in changed:
                continue
            if any(d in changed for d in pdeps):
                added.add(p)
        if not added:
            break
        changed |= added
    return changed


def strict_target(pkg: str) -> str:
    if pkg == 'http_client':
        return 'http-client-strict-ci'
    return f'{pkg}-strict-ci'


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument('--base-ref', default='HEAD~1')
    args = ap.parse_args()

    deps = json.loads(DEPS.read_text(encoding='utf-8'))
    files = get_changed(args.base_ref)
    changed_pkgs: set[str] = set()
    for f in files:
        parts = f.split('/')
        if len(parts) >= 4 and parts[0] == 'src' and parts[1] == 'vitte' and parts[2] == 'packages':
            changed_pkgs.add(parts[3])
        if len(parts) >= 4 and parts[0] == 'tests' and parts[1] == 'modules' and parts[2] == 'contracts':
            changed_pkgs.add(parts[3])

    if not changed_pkgs:
        print('packages-only-ci')
        return 0

    impacted = reverse_closure(changed_pkgs, deps)
    targets = ['facade-role-contracts-lint', 'facade-thin-lint', 'diag-namespace-lint']
    targets.extend(sorted(strict_target(p) for p in impacted if p in deps))
    print(' '.join(targets))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
