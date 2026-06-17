#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = ROOT / "docs" / "spec" / "normative.md"
OUT = ROOT / "target" / "reports" / "spec_normative_coverage.md"

REQUIRED = [
    "Memory Model",
    "Modules And Imports",
    "ABI",
    "Unsafe",
    "Traits And Generics",
    "Effects And Async",
    "Macros And Comptime",
    "Version Compatibility",
]


def main() -> int:
    if not SPEC.exists():
        print(f"[spec-normative][error] missing {SPEC.relative_to(ROOT)}")
        return 1
    text = SPEC.read_text(encoding="utf-8")
    missing = [name for name in REQUIRED if f"## {name}" not in text]
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(
        "# Normative Spec Coverage\n\n"
        + "\n".join(f"- {name}: {'PASS' if name not in missing else 'FAIL'}" for name in REQUIRED)
        + "\n",
        encoding="utf-8",
    )
    if missing:
        print(f"[spec-normative][error] missing sections: {', '.join(missing)}")
        return 1
    print(f"[spec-normative] OK sections={len(REQUIRED)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
