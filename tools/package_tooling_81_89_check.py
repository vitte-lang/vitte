#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "target" / "reports" / "package_tooling_81_89.json"


REQS = [
    (81, "minimal_package_manager", ["src/vitte/tools/package_manager/mod.vit", "tools/package_manager/run_checks.py", "Makefile"], ["minimal_package_manager_enabled", "package-manager-gate"]),
    (82, "local_offline_registry", ["src/vitte/tools/package_manager/mod.vit", "tools/package_manager/generate_artifacts.py"], ["local_offline_registry", "offline_registry.json"]),
    (83, "deterministic_lockfile", ["src/vitte/tools/package_manager/mod.vit", "tools/package_manager/generate_artifacts.py"], ["deterministic_lockfile", "vitte-lock-v1"]),
    (84, "workspace_multi_package", ["src/vitte/tools/package_manager/mod.vit", "tools/package_manager/generate_artifacts.py"], ["package_workspace", "workspace-b"]),
    (85, "dependency_resolution", ["src/vitte/tools/package_manager/mod.vit", "tools/package_manager/run_checks.py"], ["resolve_dependencies", "dependency_resolution_enabled"]),
    (86, "incremental_build_cache", ["src/vitte/tools/package_manager/mod.vit", "tools/package_manager/generate_artifacts.py"], ["incremental_build_cache_key", "build_cache.db"]),
    (87, "parallel_build_graph", ["src/vitte/tools/package_manager/mod.vit", "tools/package_manager/generate_artifacts.py"], ["parallel_build_graph", "build_graph.json"]),
    (88, "complete_lsp_goto_rename_completion_hover", ["tools/lsp/run_checks.py", "tools/lsp/generate_artifacts.py", "src/vitte/tools/lsp/mod.vit", "Makefile"], ["goto_definition", "rename: true", "completion_provider", "hover_provider"]),
    (89, "official_formatter", ["tools/vitte_format.py", "Makefile"], ["Format Vitte source files deterministically", "format-check"]),
]


def read_evidence(paths: list[str]) -> str:
    chunks: list[str] = []
    for item in paths:
        path = ROOT / item
        if path.is_file():
            chunks.append(path.read_text(encoding="utf-8", errors="replace"))
        elif path.is_dir():
            chunks.append("\n".join(str(p.relative_to(ROOT)) for p in path.rglob("*") if p.is_file()))
    return "\n".join(chunks)


def main() -> int:
    failures: list[str] = []
    rows: list[dict[str, object]] = []
    for req_id, name, evidence, needles in REQS:
        missing = [item for item in evidence if not (ROOT / item).exists()]
        text = read_evidence(evidence)
        absent = [needle for needle in needles if needle.lower() not in text.lower()]
        covered = not missing and not absent
        if missing:
            failures.append(f"{req_id} {name}: missing evidence {', '.join(missing)}")
        if absent:
            failures.append(f"{req_id} {name}: missing proof terms {', '.join(absent)}")
        rows.append({
            "id": req_id,
            "name": name,
            "evidence": evidence,
            "required_terms": needles,
            "missing_evidence": missing,
            "missing_terms": absent,
            "covered": covered,
        })

    payload = {
        "schema": "vitte.package_tooling_81_89",
        "schema_version": "1.0.0",
        "status": "pass" if not failures else "fail",
        "requirements": rows,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    if failures:
        for failure in failures:
            print(f"[package-tooling-81-89][error] {failure}", file=sys.stderr)
        print(f"[package-tooling-81-89] report={REPORT.relative_to(ROOT)}", file=sys.stderr)
        return 1
    print(f"[package-tooling-81-89] OK requirements=81-89 report={REPORT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
