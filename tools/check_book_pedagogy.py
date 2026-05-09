#!/usr/bin/env python3
from pathlib import Path
import argparse
import json
import re, sys

ROOT = Path('docs/book')
ALL = sorted(ROOT.rglob('*.html'))
SCOPE_FILE = Path('tools/book_pedagogy_scope.json')

def load_scope(phase_arg: str | None):
    data = json.loads(SCOPE_FILE.read_text(encoding='utf-8'))
    phase = phase_arg or data.get('default_phase', 'phase1')
    cfg = data['phases'].get(phase)
    if not cfg:
        raise SystemExit(f'unknown phase: {phase}')
    return phase, cfg

def is_target(p: Path, include_patterns: list[str]) -> bool:
    p2 = Path(p.as_posix())
    for pat in include_patterns:
        if p2.match(pat):
            return True
    return False

def eval_page(p: Path, strict_h1: bool):
    s = p.read_text(encoding='utf-8', errors='ignore')
    if re.search(r'<meta[^>]+http-equiv=["\']refresh["\']', s, re.I):
        return [], []
    blocking = []
    warnings = []
    h1_count = len(re.findall(r'<h1\b', s, re.I))
    if h1_count == 0:
        blocking.append('missing H1')
    elif h1_count != 1 and strict_h1:
        blocking.append('must have exactly one H1')
    elif h1_count != 1:
        warnings.append('should have exactly one H1 (progressive warning)')
    if 'TL;DR' not in s and 'chapter-tldr' not in s:
        blocking.append('missing TL;DR')
    if 'Next best action' not in s and 'chapter-cta' not in s:
        blocking.append('missing conclusion/cta')
    if '<meta name="viewport"' not in s:
        blocking.append('missing viewport meta')
    if '<title>' not in s:
        blocking.append('missing title')
    for m in re.finditer(r'<img\b[^>]*>', s, re.I):
        if ' alt=' not in m.group(0):
            warnings.append('img without alt')
            break
    return blocking, warnings

ap = argparse.ArgumentParser()
ap.add_argument('--phase', choices=['phase1', 'phase2', 'phase3'])
args = ap.parse_args()
phase_name, phase_cfg = load_scope(args.phase)
include_patterns = phase_cfg.get('include', [])
strict_h1 = bool(phase_cfg.get('strict_h1', False))

blocking = []
warnings = []

for p in ALL:
    blocking_issues, warning_issues = eval_page(p, strict_h1=strict_h1)
    if not blocking_issues and not warning_issues:
        continue
    if blocking_issues:
        line = f"{p}: " + '; '.join(blocking_issues)
        if is_target(p, include_patterns):
            blocking.append(line)
        else:
            warnings.append(f"{line} [non-blocking outside target]")
    if warning_issues:
        wline = f"{p}: " + '; '.join(warning_issues)
        warnings.append(wline)

if warnings:
    print('book pedagogy warnings (outside target subset):')
    for w in warnings[:250]:
        print('-', w)

if blocking:
    print(f'book pedagogy check failed ({phase_name})')
    for b in blocking[:250]:
        print('-', b)
    sys.exit(1)

print(f'book pedagogy check ok ({phase_name})')
print(f'non-blocking warnings count: {len(warnings)}')
