#!/usr/bin/env python3
"""Generate lightweight railroad SVG diagrams from the code grammar source."""

from __future__ import annotations

import argparse
import html
import re
from dataclasses import dataclass
from pathlib import Path


RULE_RE = re.compile(r"^([a-zA-Z_][a-zA-Z0-9_]*)\s*::=\s*(.*)$")


@dataclass(frozen=True)
class Rule:
    name: str
    rhs: str


def parse_rules(text: str) -> list[Rule]:
    rules: list[Rule] = []
    current_name: str | None = None
    current_rhs: list[str] = []

    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#") or line.startswith("<<<"):
            continue

        if current_name is None:
            m = RULE_RE.match(line)
            if not m:
                continue
            current_name = m.group(1)
            tail = m.group(2)
            if tail.endswith(";"):
                rules.append(Rule(current_name, tail[:-1].strip()))
                current_name = None
                current_rhs = []
            else:
                current_rhs = [tail]
            continue

        # Continuation line of the current rule.
        if line.endswith(";"):
            current_rhs.append(line[:-1].strip())
            rules.append(Rule(current_name, " ".join(current_rhs).strip()))
            current_name = None
            current_rhs = []
        else:
            current_rhs.append(line)

    return rules


def split_alternatives(rhs: str) -> list[str]:
    out: list[str] = []
    buf: list[str] = []
    depth_round = depth_square = depth_curly = 0
    in_string = False

    for ch in rhs:
        if ch == '"':
            in_string = not in_string
            buf.append(ch)
            continue
        if not in_string:
            if ch == '(':
                depth_round += 1
            elif ch == ')':
                depth_round = max(0, depth_round - 1)
            elif ch == '[':
                depth_square += 1
            elif ch == ']':
                depth_square = max(0, depth_square - 1)
            elif ch == '{':
                depth_curly += 1
            elif ch == '}':
                depth_curly = max(0, depth_curly - 1)
            elif ch == '|' and depth_round == 0 and depth_square == 0 and depth_curly == 0:
                part = "".join(buf).strip()
                if part:
                    out.append(part)
                buf = []
                continue
        buf.append(ch)

    tail = "".join(buf).strip()
    if tail:
        out.append(tail)
    return out or [rhs.strip()]


def draw_rule_svg(rule: Rule) -> str:
    alts = split_alternatives(rule.rhs)
    max_chars = max((len(a) for a in alts), default=10)
    width = max(780, 220 + max_chars * 7)
    row_h = 44
    top_pad = 52
    height = top_pad + len(alts) * row_h + 22

    lines: list[str] = []
    lines.append('<?xml version="1.0" encoding="UTF-8"?>')
    lines.append(
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">'
    )
    lines.append(
        '<style>'
        'text{font-family:ui-monospace,SFMono-Regular,Menlo,Consolas,monospace;fill:#10243a;}'
        '.title{font-size:18px;font-weight:700;}'
        '.alt{font-size:13px;}'
        '.box{fill:#f6fbff;stroke:#4a90c2;stroke-width:1.2;rx:8;ry:8;}'
        '.line{stroke:#4a90c2;stroke-width:2;fill:none;}'
        '.dot{fill:#4a90c2;}'
        '</style>'
    )
    lines.append(f'<text x="20" y="30" class="title">{html.escape(rule.name)}</text>')

    box_x = 96
    box_w = width - 170
    for idx, alt in enumerate(alts):
        y = top_pad + idx * row_h
        cy = y + 14
        lines.append(f'<circle class="dot" cx="30" cy="{cy}" r="5" />')
        lines.append(f'<path class="line" d="M 35 {cy} L {box_x} {cy}" />')
        lines.append(f'<rect class="box" x="{box_x}" y="{y}" width="{box_w}" height="28" />')
        lines.append(f'<text class="alt" x="{box_x + 10}" y="{y + 19}">{html.escape(alt)}</text>')
        lines.append(f'<path class="line" d="M {box_x + box_w} {cy} L {width - 30} {cy}" />')
        lines.append(f'<circle class="dot" cx="{width - 24}" cy="{cy}" r="5" />')

    lines.append('</svg>')
    return "\n".join(lines) + "\n"


def build_index(rules: list[Rule]) -> str:
    lines = ["# Railroad Diagrams", "", "Generated from `src/vitte/grammar/vitte.ebnf`.", ""]
    for rule in rules:
        lines.append(f"- [{rule.name}](./{rule.name}.svg)")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="Build railroad SVG diagrams from src/vitte/grammar/vitte.ebnf")
    parser.add_argument("--check", action="store_true", help="fail if generated files are out of sync")
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[3]
    grammar_path = repo / "src/vitte/grammar/vitte.ebnf"
    out_dir = repo / "book/grammar/railroad"
    index_path = out_dir / "README.md"

    if not grammar_path.exists():
        print(f"[railroad] missing input: {grammar_path}")
        return 1

    text = grammar_path.read_text(encoding="utf-8")
    rules = parse_rules(text)
    if not rules:
        print("[railroad] no rules parsed")
        return 1

    mismatches: list[Path] = []
    writes: list[tuple[Path, str]] = []

    for rule in rules:
        target = out_dir / f"{rule.name}.svg"
        content = draw_rule_svg(rule)
        writes.append((target, content))
        current = target.read_text(encoding="utf-8") if target.exists() else ""
        if current != content:
            mismatches.append(target)

    index_content = build_index(rules)
    writes.append((index_path, index_content))
    current_index = index_path.read_text(encoding="utf-8") if index_path.exists() else ""
    if current_index != index_content:
        mismatches.append(index_path)

    if args.check:
        if mismatches:
            print("[railroad] FAILED")
            for p in mismatches:
                print(f"- out of sync: {p}")
            return 1
        print("[railroad] OK")
        return 0

    out_dir.mkdir(parents=True, exist_ok=True)
    for path, content in writes:
        path.write_text(content, encoding="utf-8")
        print(f"[railroad] wrote {path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
