#!/usr/bin/env python3
from __future__ import annotations
import json, re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CFG = ROOT / "tools/facade_packages.json"
CODE_RE = re.compile(r'if\s+code\s*==\s*"(VITTE-[A-Z][0-9]{4})"\s*\{\s*give\s*"([^"]+)"')


def collect_from_file(path: Path):
    text = path.read_text(encoding="utf-8")
    out = {}
    for code, msg in CODE_RE.findall(text):
        out[code] = msg
    return out


def main() -> int:
    cfg = json.loads(CFG.read_text(encoding="utf-8"))
    idx = {}
    for ent in cfg["packages"]:
        pkg = ent["name"]
        root = ROOT / f"src/vitte/packages/{pkg}"
        msgs = {}
        fixes = {}
        for p in root.rglob("*.vit"):
            d = collect_from_file(p)
            for k, v in d.items():
                if "quickfix" in p.name:
                    fixes[k] = v
                else:
                    msgs[k] = v
        # fallback: include all as messages
        for p in root.rglob("*.vit"):
            txt = p.read_text(encoding="utf-8")
            for m in re.finditer(r'"(VITTE-[A-Z][0-9]{4})"', txt):
                c = m.group(1)
                msgs.setdefault(c, "")
        for code in sorted(set(msgs) | set(fixes)):
            idx[code] = {
                "package": pkg,
                "message": msgs.get(code, ""),
                "quickfix": fixes.get(code, ""),
                "doc_url": f"docs/{pkg}/DIAGNOSTICS.md",
            }
    out_json = ROOT / "target/reports/diagnostics_index.json"
    out_md = ROOT / "target/reports/diagnostics_index.md"
    out_json.parent.mkdir(parents=True, exist_ok=True)
    out_json.write_text(json.dumps(idx, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    lines = ["# Diagnostics Index", "", "| Code | Package | Message | Quick-fix | Doc |", "|---|---|---|---|---|"]
    for code in sorted(idx):
        e = idx[code]
        lines.append(f"| {code} | {e['package']} | {e['message']} | {e['quickfix']} | {e['doc_url']} |")
    out_md.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"[diagnostics-index] wrote {out_json}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
