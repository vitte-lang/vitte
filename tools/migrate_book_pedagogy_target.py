#!/usr/bin/env python3
from pathlib import Path
import argparse
import json
import re

ROOT = Path('docs/book')
SCOPE_FILE = Path('tools/book_pedagogy_scope.json')

def load_scope(phase_arg: str | None):
    data = json.loads(SCOPE_FILE.read_text(encoding='utf-8'))
    phase = phase_arg or data.get('default_phase', 'phase1')
    cfg = data['phases'].get(phase)
    if not cfg:
        raise SystemExit(f'unknown phase: {phase}')
    return phase, cfg

def target_files(include_patterns: list[str]):
    out=[]
    for p in sorted(ROOT.rglob('*.html')):
        p2 = Path(p.as_posix())
        for pat in include_patterns:
            if p2.match(pat):
                out.append(p)
                break
    return out

TLDR = '<section class="chapter-tldr"><h2>TL;DR (5 lines)</h2><ul><li>Read the core idea first.</li><li>Understand one concept at a time.</li><li>Run a small example.</li><li>Fix one issue at a time.</li><li>Apply before moving on.</li></ul></section>'
CTA = '<section class="chapter-cta"><h2>Next best action</h2><p>Apply one key idea from this chapter in a tiny example, then continue.</p></section>'

ap = argparse.ArgumentParser()
ap.add_argument('--phase', choices=['phase1', 'phase2', 'phase3'])
args = ap.parse_args()
phase_name, phase_cfg = load_scope(args.phase)

for p in target_files(phase_cfg.get('include', [])):
    s=p.read_text(encoding='utf-8',errors='ignore')
    if re.search(r'<meta[^>]+http-equiv=["\']refresh["\']', s, re.I):
      continue
    # ensure single H1 fallback
    if len(re.findall(r'<h1\b', s, re.I))==0:
      injected = False
      if '<article class="doc-content">' in s:
        s=s.replace('<article class="doc-content">','<article class="doc-content">\n<h1>'+p.stem+'</h1>',1)
        injected = True
      if not injected:
        s=re.sub(r'(<main\b[^>]*>)', r'\1\n<h1>'+p.stem+'</h1>', s, count=1, flags=re.I)
    # ensure viewport
    if '<meta name="viewport"' not in s:
      s=s.replace('<meta charset="utf-8">','<meta charset="utf-8">\n<meta name="viewport" content="width=device-width, initial-scale=1">',1)
    # inject TLDR after first h1
    if 'chapter-tldr' not in s and 'TL;DR' not in s:
      if re.search(r'</h1>', s, re.I):
        s=re.sub(r'(</h1>)', r'\1\n'+TLDR, s, count=1, flags=re.I)
      elif re.search(r'<main\b[^>]*>', s, re.I):
        s=re.sub(r'(<main\b[^>]*>)', r'\1\n'+TLDR, s, count=1, flags=re.I)
      else:
        s=s.replace('</body>', TLDR+'\n</body>')
    # inject CTA before end of article
    if 'chapter-cta' not in s and 'Next best action' not in s:
      if '</article>' in s:
        s=s.replace('</article>', CTA+'\n</article>')
      elif re.search(r'</main>', s, re.I):
        s=re.sub(r'</main>', CTA+'\n</main>', s, count=1, flags=re.I)
      else:
        s=s.replace('</body>', CTA+'\n</body>')
    # ensure img alt
    s=re.sub(r'<img(?![^>]*\salt=)([^>]*)>', r'<img alt=""\1>', s, flags=re.I)
    p.write_text(s,encoding='utf-8')

print(f'migrated pedagogy target subset ({phase_name})')
