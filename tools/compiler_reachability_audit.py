#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path
from collections import deque

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src" / "vitte" / "compiler"
ENTRY = "vitte/compiler/driver/compiler"
ALLOW = ROOT / "tools" / "compiler_reachability_allowlist.txt"

USE_RE = re.compile(r"^\s*use\s+([a-zA-Z0-9_/]+)")


def module_id(path: Path) -> str:
    rel = path.relative_to(ROOT / "src").with_suffix("")
    return str(rel).replace("\\", "/")


def load_allow() -> set[str]:
    out: set[str] = set()
    if not ALLOW.exists():
        return out
    for raw in ALLOW.read_text(encoding="utf-8").splitlines():
        s = raw.strip()
        if not s or s.startswith("#"):
            continue
        out.add(s)
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
    unreachable = sorted(m for m in modules if m not in seen and m not in allow)

    print(f"[compiler-reachability] modules={len(modules)} reachable={len(seen)} allowlisted={len(allow)} unreachable={len(unreachable)}")
    if unreachable:
        for m in unreachable:
            print(f"- {m}")
        return 1

    print("[compiler-reachability] ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
