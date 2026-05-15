#!/usr/bin/env python3
import argparse
import json
import re
import shutil
import subprocess
import tempfile
from dataclasses import dataclass
from pathlib import Path

ROOT = Path('.').resolve()
EXPL = ROOT / 'locales/en/diagnostics_explain.ftl'


@dataclass
class Proposal:
    title: str
    confidence: int
    before: str
    after: str
    reason: str


def parse_ftl(p: Path) -> dict[str, str]:
    d: dict[str, str] = {}
    if not p.exists():
        return d
    for ln in p.read_text(encoding='utf-8', errors='ignore').splitlines():
        if '=' in ln and not ln.strip().startswith('#'):
            k, v = ln.split('=', 1)
            d[k.strip()] = v.strip()
    return d


FTL = parse_ftl(EXPL)


def classify(msg: str) -> str:
    s = msg.lower()
    if any(k in s for k in ['expected', 'parser', 'syntax']):
        return 'syntax'
    if any(k in s for k in ['unknown type', 'type mismatch', 'generic type', 'cannot infer']):
        return 'type-system'
    if any(k in s for k in ['import', 'module', 'path', 'alias']):
        return 'module-resolution'
    if any(k in s for k in ['hir', 'lowering', 'mir']):
        return 'compiler-lowering'
    if any(k in s for k in ['borrow', 'lifetime', 'ownership']):
        return 'ownership'
    return 'general'


def parse_position(log_text: str):
    patterns = [
        re.compile(r'(?P<file>[^:\n]+\.(?:vit|vitl)):(?P<line>\d+):(?P<col>\d+)'),
        re.compile(r'line\s+(?P<line>\d+)[,:]\s*column\s+(?P<col>\d+)', re.IGNORECASE),
        re.compile(r'line\s+(?P<line>\d+)', re.IGNORECASE),
    ]
    for pat in patterns:
        m = pat.search(log_text)
        if m:
            line = int(m.groupdict().get('line') or 1)
            col = int(m.groupdict().get('col') or 1)
            file_part = m.groupdict().get('file')
            return file_part, line, col
    return None, None, None


def extract_error_lines(log_text: str) -> list[str]:
    lines = [ln for ln in log_text.splitlines() if ln.strip()]
    interesting = [ln for ln in lines if re.search(r'\berror\b|\bE\d{4}\b|expected|unknown|failed', ln, re.I)]
    return interesting[-8:] if interesting else lines[-8:]


def root_cause_rank(lines: list[str]) -> list[str]:
    weights = [
        (r'expected .*|parser|syntax', 100),
        (r'unknown (identifier|type)|module .* not found|import', 90),
        (r'type mismatch|cannot infer|generic', 80),
        (r'hir|mir|lowering|internal compiler', 70),
    ]
    scored = []
    for ln in lines:
        s = 10
        for pat, w in weights:
            if re.search(pat, ln, re.I):
                s = max(s, w)
        scored.append((s, ln))
    scored.sort(reverse=True, key=lambda x: x[0])
    return [ln for _, ln in scored[:3]]


def render_snippet(src: Path, line: int, col: int, radius: int = 4) -> str:
    lines = src.read_text(encoding='utf-8', errors='ignore').splitlines()
    if not lines:
        return '(empty source)'
    line = max(1, min(line, len(lines)))
    col = max(1, col)
    start = max(1, line - radius)
    end = min(len(lines), line + radius)
    out: list[str] = []
    for i in range(start, end + 1):
        marker = '>>' if i == line else '  '
        out.append(f'{marker} {i:5d} | {lines[i - 1]}')
        if i == line:
            out.append(f'         | ' + ' ' * (col - 1) + '^')
    return '\n'.join(out)


def propose_patches(src: Path, log_text: str, line: int | None, col: int | None) -> list[Proposal]:
    proposals: list[Proposal] = []
    text = src.read_text(encoding='utf-8', errors='ignore')
    lines = text.splitlines()
    low = log_text.lower()
    if not line or line < 1 or line > len(lines):
        return proposals

    target = lines[line - 1]

    if 'expected' in low and '}' in low and not target.rstrip().endswith('}'):
        proposals.append(Proposal('Close missing block delimiter', 92, target, target + ' }', 'Parser expects closing delimiter.'))
    if ('unknown identifier' in low or 'unknown type' in low) and not re.search(r'^\s*use\s+', text, re.M):
        proposals.append(Proposal('Add likely missing import', 90, target, 'use std/core\n' + target, 'No import found in file for unresolved symbol.'))
    if re.search(r'\bE0013\b', log_text) or 'without returning a value' in low:
        proposals.append(Proposal('Add explicit return on path', 93, target, target + '\n  give 0', 'Non-void proc has path without return.'))
    if 'expected identifier' in low and col and col > 1:
        fixed = target[:col - 1] + 'name' + target[col - 1:]
        proposals.append(Proposal('Insert placeholder identifier', 91, target, fixed, 'Token expected where identifier should appear.'))

    return proposals


def validate_patch(src: Path, proposal: Proposal, compiler: str = 'bin/vittec0') -> tuple[bool, str]:
    with tempfile.TemporaryDirectory(prefix='vitte-brain-') as td:
        tmp = Path(td) / src.name
        tmp.write_text(src.read_text(encoding='utf-8', errors='ignore'), encoding='utf-8')
        original = tmp.read_text(encoding='utf-8', errors='ignore')
        if proposal.before not in original:
            return False, 'pattern not found in source'
        tmp.write_text(original.replace(proposal.before, proposal.after, 1), encoding='utf-8')
        cmd = [compiler, 'check', '--strict', str(tmp)]
        r = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        if r.returncode == 0:
            return True, 'check --strict passed on patched temp file'
        return False, (r.stdout or 'check failed')[-500:]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument('log')
    ap.add_argument('src', nargs='?')
    ap.add_argument('--json-out', default='')
    ap.add_argument('--autofix-safe', action='store_true')
    args = ap.parse_args()

    logp = Path(args.log)
    src = Path(args.src) if args.src else None
    txt = logp.read_text(encoding='utf-8', errors='ignore') if logp.exists() else ''
    code_match = re.search(r'\b(E\d{4})\b', txt)
    code = code_match.group(1) if code_match else None
    kind = classify(txt)

    file_hint, line, col = parse_position(txt)
    if (src is None or not src.exists()) and file_hint:
        maybe = Path(file_hint)
        if maybe.exists():
            src = maybe

    err_lines = extract_error_lines(txt)
    root_causes = root_cause_rank(err_lines)

    summary = FTL.get(f'{code}.summary', '') if code else ''
    fix = FTL.get(f'{code}.fix', '') if code else ''
    example = FTL.get(f'{code}.example', '') if code else ''

    proposals: list[Proposal] = []
    validations = []
    if src and src.exists():
        proposals = propose_patches(src, txt, line, col)
        if args.autofix_safe:
            for p in proposals:
                ok, msg = validate_patch(src, p)
                validations.append({'title': p.title, 'ok': ok, 'details': msg})

    print('================ VITTE BRAIN DIAGNOSTICS ================')
    print(f'error_class: {kind}')
    if code:
        print(f'error_code: {code}')
    if summary:
        print(f'summary: {summary}')
    if fix:
        print(f'fluent_fix: {fix}')
    if example:
        print(f'example: {example}')
    if line:
        print(f'position: line={line} col={col or 1}')

    print('root_causes:')
    for i, rc in enumerate(root_causes, 1):
        print(f'{i}) {rc}')

    print('raw_tail:')
    for ln in txt.splitlines()[-30:]:
        print(ln)

    if src and src.exists() and line:
        print('source_snippet:')
        print(render_snippet(src, line, col or 1))

    if proposals:
        print('auto_patch_candidates:')
        for i, p in enumerate(proposals, 1):
            print(f'{i}) {p.title} (confidence={p.confidence}%)')
            print(f'   reason: {p.reason}')
            print(f'   before: {p.before}')
            print(f'   after : {p.after}')
        if validations:
            print('auto_patch_validation:')
            for v in validations:
                st = 'PASS' if v['ok'] else 'FAIL'
                print(f"- {st} {v['title']}: {v['details']}")

    print('suggestions_prioritized:')
    suggestions = [
        '1) Fix the first root cause above before all others (anti-cascade).',
        '2) Re-run strict parser: bin/vittec0 parse "<file>".',
        '3) Re-run strict check: bin/vittec0 check --strict "<file>".',
        '4) Trace pipeline: bin/vittec0 --trace-pipeline check --strict "<file>".',
        '5) Validate module imports/aliases and canonical paths.',
        '6) Run cli diagnostics snapshots: make cli-diagnostics-snapshots.',
        '7) Run explain snapshots: make explain-snapshots.',
    ]
    for s in suggestions:
        print(s)
    print('=========================================================')

    payload = {
        'error_class': kind,
        'error_code': code,
        'position': {'line': line, 'col': col},
        'root_causes': root_causes,
        'fluent': {'summary': summary, 'fix': fix, 'example': example},
        'proposals': [p.__dict__ for p in proposals],
        'validations': validations,
    }
    if args.json_out:
        out = Path(args.json_out)
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(json.dumps(payload, indent=2, ensure_ascii=False), encoding='utf-8')

    return 0


if __name__ == '__main__':
    raise SystemExit(main())
