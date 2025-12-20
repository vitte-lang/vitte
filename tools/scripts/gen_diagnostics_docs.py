#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable


@dataclass(frozen=True)
class Example:
    bad: str
    good: str


@dataclass(frozen=True)
class Diagnostic:
    code: str
    severity: str
    name: str
    message: str
    help: str
    examples: tuple[Example, ...]


def _require_str(obj: dict[str, Any], key: str) -> str:
    v = obj.get(key)
    if not isinstance(v, str) or not v.strip():
        raise ValueError(f"diagnostic.{key} must be a non-empty string")
    return v


def _parse_examples(examples: Any) -> tuple[Example, ...]:
    if examples is None:
        return ()
    if not isinstance(examples, list):
        raise ValueError("diagnostic.examples must be a list")
    out: list[Example] = []
    for ex in examples:
        if not isinstance(ex, dict):
            raise ValueError("diagnostic.examples items must be objects")
        bad = _require_str(ex, "bad")
        good = _require_str(ex, "good")
        out.append(Example(bad=bad, good=good))
    return tuple(out)


def load_registry(path: Path) -> list[Diagnostic]:
    data = json.loads(path.read_text(encoding="utf-8"))
    diags_raw = data.get("diagnostics")
    if not isinstance(diags_raw, list):
        raise ValueError("registry.diagnostics must be a list")

    seen: set[str] = set()
    out: list[Diagnostic] = []
    for d in diags_raw:
        if not isinstance(d, dict):
            raise ValueError("diagnostics items must be objects")
        code = _require_str(d, "code")
        if code in seen:
            raise ValueError(f"duplicate diagnostic code: {code}")
        seen.add(code)

        sev = _require_str(d, "severity")
        if sev not in ("error", "warning", "note"):
            raise ValueError(f"invalid severity for {code}: {sev}")

        out.append(
            Diagnostic(
                code=code,
                severity=sev,
                name=_require_str(d, "name"),
                message=_require_str(d, "message"),
                help=_require_str(d, "help"),
                examples=_parse_examples(d.get("examples")),
            )
        )
    out.sort(key=lambda x: (x.severity, x.code))
    return out


def _group_by_severity(diags: Iterable[Diagnostic]) -> dict[str, list[Diagnostic]]:
    out: dict[str, list[Diagnostic]] = {"error": [], "warning": [], "note": []}
    for d in diags:
        out.setdefault(d.severity, []).append(d)
    return out


def render_markdown(diags: list[Diagnostic], registry_path: Path) -> str:
    groups = _group_by_severity(diags)

    def _render_section(title: str, items: list[Diagnostic]) -> str:
        if not items:
            return ""
        lines: list[str] = []
        lines.append(f"## {title}")
        lines.append("")
        lines.append("| Code | Name | Message |")
        lines.append("|---|---|---|")
        for d in items:
            lines.append(f"| `{d.code}` | `{d.name}` | {d.message} |")
        lines.append("")
        for d in items:
            lines.append(f"### `{d.code}` `{d.name}`")
            lines.append("")
            lines.append(f"- Message: {d.message}")
            lines.append(f"- Help: {d.help}")
            if d.examples:
                lines.append("- Examples:")
                for ex in d.examples:
                    lines.append("")
                    lines.append("```vitte")
                    lines.append(ex.bad.rstrip("\n"))
                    lines.append("```")
                    lines.append("")
                    lines.append("```vitte")
                    lines.append(ex.good.rstrip("\n"))
                    lines.append("```")
            lines.append("")
        return "\n".join(lines)

    parts: list[str] = []
    parts.append("# Diagnostics")
    parts.append("")
    parts.append(
        "This page is generated. Edit the registry and re-run the generator:"
    )
    parts.append("")
    parts.append(f"- Registry: `{registry_path.as_posix()}`")
    parts.append(
        "- Regenerate: `python3 tools/scripts/gen_diagnostics_docs.py`"
    )
    parts.append("")

    parts.append(_render_section("Errors", groups.get("error", [])))
    parts.append(_render_section("Warnings", groups.get("warning", [])))
    parts.append(_render_section("Notes", groups.get("note", [])))

    return "\n".join([p for p in parts if p])


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "--registry",
        default="docs/diagnostics/registry.json",
        help="Path to diagnostics registry JSON",
    )
    ap.add_argument(
        "--out",
        default="docs/diagnostics.md",
        help="Path to output markdown file",
    )
    args = ap.parse_args()

    registry_path = Path(args.registry)
    out_path = Path(args.out)
    diags = load_registry(registry_path)
    md = render_markdown(diags, registry_path)
    out_path.write_text(md, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

