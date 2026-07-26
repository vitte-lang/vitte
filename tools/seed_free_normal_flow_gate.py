#!/usr/bin/env python3
from __future__ import annotations

import json
import re
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
MAKEFILE = ROOT / "Makefile"
REPORT_DIR = ROOT / "target/reports"
REPORT_JSON = REPORT_DIR / "seed_free_normal_flow_gate.json"
REPORT_MD = REPORT_DIR / "seed_free_normal_flow_gate.md"

NORMAL_TARGET_PREFIXES = ("release-", "selfhost-", "package-", "stdlib-")
ALLOWED_TARGET_PREFIXES = ("seed-", "bootstrap-")
FORBIDDEN_RECIPE_MARKERS = (
    "bin/vittec0",
    "toolchain/seed",
    "scripts/seed/install_seed.sh",
    "scripts/seed/verify_seed.sh",
    "vittec0.seed",
)
FORBIDDEN_DIRECT_PREREQUISITES = (
    "build",
    "bootstrap-all",
    "bootstrap-seed",
    "seed-gate",
    "seed-check",
    "seed-verify",
    "seed-install",
)
SCRIPT_SCAN_ROOTS = ("scripts", "scripts_build")
SCRIPT_FORBIDDEN_MARKERS = (
    "toolchain/seed",
    "scripts/seed/install_seed.sh",
    "scripts/seed/verify_seed.sh",
    "vittec0.seed",
)


TARGET_RE = re.compile(r"^([A-Za-z0-9_.%/+ -]+):([^=].*)?$")


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def is_normal_target(name: str) -> bool:
    return (name == "build" or name.startswith(NORMAL_TARGET_PREFIXES)) and not name.startswith(ALLOWED_TARGET_PREFIXES)


def parse_makefile() -> list[dict[str, Any]]:
    lines = MAKEFILE.read_text(encoding="utf-8").splitlines()
    targets: list[dict[str, Any]] = []
    current: dict[str, Any] | None = None
    for index, line in enumerate(lines, start=1):
        if line.startswith("\t"):
            if current is not None:
                current["recipe"].append({"line": index, "text": line})
            continue
        match = TARGET_RE.match(line)
        if not match:
            current = None
            continue
        names = [part for part in match.group(1).split() if part != ".PHONY"]
        prereq_text = match.group(2) or ""
        prereqs = [part for part in prereq_text.strip().split() if part and not part.startswith("|")]
        for name in names:
            current = {"name": name, "line": index, "prereqs": prereqs, "recipe": []}
            targets.append(current)
    return targets


def validate_makefile(failures: list[str]) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    for target in parse_makefile():
        name = str(target["name"])
        if not is_normal_target(name):
            continue
        row_failures: list[str] = []
        for prereq in target["prereqs"]:
            if prereq in FORBIDDEN_DIRECT_PREREQUISITES:
                row_failures.append(f"line {target['line']}: direct prerequisite {prereq}")
        for recipe in target["recipe"]:
            text = str(recipe["text"])
            for marker in FORBIDDEN_RECIPE_MARKERS:
                if marker in text:
                    row_failures.append(f"line {recipe['line']}: recipe contains {marker}")
        if row_failures:
            failures.extend(f"normal target {name}: {failure}" for failure in row_failures)
        rows.append({"name": name, "line": target["line"], "prereqs": target["prereqs"], "failures": row_failures})
    return rows


def script_allowed(path: Path) -> bool:
    parts = path.relative_to(ROOT).parts
    return len(parts) >= 2 and parts[1] == "seed"


def validate_scripts(failures: list[str]) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    for root_name in SCRIPT_SCAN_ROOTS:
        root = ROOT / root_name
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*.sh")):
            if script_allowed(path):
                continue
            text = path.read_text(encoding="utf-8", errors="replace")
            hits = [marker for marker in SCRIPT_FORBIDDEN_MARKERS if marker in text]
            if hits:
                failures.append(f"{rel(path)} contains normal-flow seed dependency: {', '.join(hits)}")
            rows.append({"path": rel(path), "forbidden_hits": hits})
    return rows


def write_reports(status: str, make_targets: list[dict[str, Any]], scripts: list[dict[str, Any]], failures: list[str]) -> None:
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    payload = {
        "schema": "vitte.seed.free.normal.flow.gate.v1",
        "status": status,
        "normal_target_prefixes": NORMAL_TARGET_PREFIXES,
        "forbidden_recipe_markers": FORBIDDEN_RECIPE_MARKERS,
        "forbidden_direct_prerequisites": FORBIDDEN_DIRECT_PREREQUISITES,
        "make_targets": make_targets,
        "scripts": scripts,
        "failures": failures,
    }
    REPORT_JSON.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    lines = [
        "# seed-free normal flow gate",
        "",
        f"- status: {status}",
        f"- make targets checked: {len(make_targets)}",
        f"- scripts checked: {len(scripts)}",
    ]
    if failures:
        lines.append("")
        lines.append("## Failures")
        lines.extend(f"- {failure}" for failure in failures)
    REPORT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    failures: list[str] = []
    make_targets = validate_makefile(failures)
    scripts = validate_scripts(failures)
    status = "fail" if failures else "pass"
    write_reports(status, make_targets, scripts, failures)
    if failures:
        print("[seed-free-normal-flow-gate][error] gate failed", file=sys.stderr)
        for failure in failures:
            print(f" - {failure}", file=sys.stderr)
        return 1
    print(f"[seed-free-normal-flow-gate] ok targets={len(make_targets)} scripts={len(scripts)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
