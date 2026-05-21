#!/usr/bin/env python3
from __future__ import annotations

import re
import json
import sys
from pathlib import Path
from collections import deque

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src" / "vitte" / "compiler"
ENTRY = "vitte/compiler/driver/compiler"
ALLOW = ROOT / "tools" / "compiler_reachability_allowlist.txt"
REPORT = ROOT / "target" / "reports" / "compiler_reachability" / "audit.json"

USE_RE = re.compile(r"^\s*use\s+([a-zA-Z0-9_/]+)")


def module_id(path: Path) -> str:
    rel = path.relative_to(ROOT / "src").with_suffix("")
    return str(rel).replace("\\", "/")


def module_area(module_name: str) -> str:
    if module_name.startswith("vitte/compiler/analysis/"):
        return "analysis"
    if module_name.startswith("vitte/compiler/middle/"):
        return "middle"
    if module_name.startswith("vitte/compiler/backend/") or module_name.startswith("vitte/compiler/backends/"):
        return "backend"
    if module_name.startswith("vitte/compiler/frontend/"):
        return "frontend"
    if module_name.startswith("vitte/compiler/infrastructure/"):
        return "infrastructure"
    if module_name.startswith("vitte/compiler/driver/"):
        return "driver"
    return "other"


def load_allow() -> dict[str, str]:
    out: dict[str, str] = {}
    if not ALLOW.exists():
        return out
    for idx, raw in enumerate(ALLOW.read_text(encoding="utf-8").splitlines(), 1):
        s = raw.strip()
        if not s or s.startswith("#"):
            continue
        if "|" not in s:
            print(f"[compiler-reachability][error] invalid allowlist entry at line {idx}: expected 'module|reason'")
            raise SystemExit(1)
        mod, reason = s.split("|", 1)
        mod = mod.strip()
        reason = reason.strip()
        if not mod or not reason:
            print(f"[compiler-reachability][error] invalid allowlist entry at line {idx}: empty module or reason")
            raise SystemExit(1)
        out[mod] = reason
    return out


def main() -> int:
    files = []
    for p in sorted(SRC.rglob("*.vit")):
        rel = p.relative_to(SRC)
        parts = rel.parts
        if "tests" in parts:
            continue
        if "frontend_fixtures" in parts:
            continue
        files.append(p)
    modules = {module_id(p): p for p in files}
    if ENTRY not in modules:
        print(f"[compiler-reachability][error] missing entry module: {ENTRY}")
        return 1

    deps: dict[str, set[str]] = {m: set() for m in modules}
    for mid, p in modules.items():
        txt = p.read_text(encoding="utf-8", errors="replace")
        for ln in txt.splitlines():
            m = USE_RE.match(ln)
            if not m:
                continue
            dep = m.group(1)
            if dep.startswith("vitte/compiler/") and dep in modules:
                deps[mid].add(dep)

    seen: set[str] = set()
    q: deque[str] = deque([ENTRY])
    while q:
        cur = q.popleft()
        if cur in seen:
            continue
        seen.add(cur)
        for nxt in sorted(deps.get(cur, ())):
            if nxt not in seen:
                q.append(nxt)

    allow = load_allow()
    allow_set = set(allow.keys())
    stale_allow = sorted(m for m in allow_set if m not in modules)
    unreachable = sorted(m for m in modules if m not in seen and m not in allow_set)

    if stale_allow:
        print(f"[compiler-reachability][error] stale allowlist entries={len(stale_allow)}")
        for m in stale_allow:
            print(f"- stale {m}")
        return 1

    areas = ("frontend", "analysis", "middle", "backend", "infrastructure", "driver", "other")
    summary: dict[str, dict[str, int]] = {}
    for area in areas:
        mods = [m for m in modules if module_area(m) == area]
        reachable = [m for m in mods if m in seen]
        allowlisted = [m for m in mods if m in allow_set]
        unknown = [m for m in mods if m in unreachable]
        summary[area] = {
            "total": len(mods),
            "reachable": len(reachable),
            "allowlisted": len(allowlisted),
            "unreachable": len(unknown),
        }

    print(f"[compiler-reachability] modules={len(modules)} reachable={len(seen)} allowlisted={len(allow_set)} unreachable={len(unreachable)}")
    for area in areas:
        block = summary[area]
        print(
            f"[compiler-reachability][area] {area} total={block['total']} "
            f"reachable={block['reachable']} allowlisted={block['allowlisted']} unreachable={block['unreachable']}"
        )

    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(
        json.dumps(
            {
                "schema": "vitte.compiler.reachability",
                "schema_version": "1.0.0",
                "entry": ENTRY,
                "modules": len(modules),
                "reachable": len(seen),
                "allowlisted": len(allow_set),
                "unreachable": len(unreachable),
                "summary_by_area": summary,
                "allowlist_reasons": allow,
                "unreachable_modules": unreachable,
            },
            ensure_ascii=True,
            indent=2,
            sort_keys=True,
        )
        + "\n",
        encoding="utf-8",
    )
    if unreachable:
        for m in unreachable:
            print(f"- {m}")
        return 1

    print(f"[compiler-reachability] ok report={REPORT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
