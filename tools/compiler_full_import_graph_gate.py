#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import re
import subprocess
import sys
import tempfile
from collections import deque
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
SOURCE_ROOT = ROOT / "src"
COMPILER_ROOT = SOURCE_ROOT / "vitte/compiler"
ENTRYPOINT = COMPILER_ROOT / "main.vit"
BOOTSTRAP = ROOT / "target/bootstrap-c17/vitte-bootstrap"
REPORT_JSON = ROOT / "target/reports/compiler_full_import_graph_gate.json"
REPORT_MD = ROOT / "target/reports/compiler_full_import_graph_gate.md"

EXPECTED_MODULES = 971
USE_RE = re.compile(
    r"^\s*(?:use|import)\s+([A-Za-z0-9_/-]+(?:\.[A-Za-z0-9_/-]+)*)",
    re.MULTILINE,
)


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def compiler_sources() -> list[Path]:
    return sorted(path for path in COMPILER_ROOT.rglob("*.vit") if path.is_file())


def primary_module_name(path: Path) -> str:
    return path.relative_to(SOURCE_ROOT).with_suffix("").as_posix()


def import_aliases(path: Path) -> set[str]:
    primary = primary_module_name(path)
    aliases = {primary}
    if path.name == "mod.vit":
        aliases.add(path.parent.relative_to(SOURCE_ROOT).as_posix())
    return aliases


def import_module_name(raw: str) -> str:
    return raw.replace(".", "/")


def build_graph(
    sources: list[Path],
    failures: list[str],
) -> tuple[dict[str, set[str]], int, dict[str, str]]:
    alias_to_primary: dict[str, str] = {}
    for path in sources:
        primary = primary_module_name(path)
        for alias in import_aliases(path):
            previous = alias_to_primary.get(alias)
            if previous is not None and previous != primary:
                failures.append(f"module alias {alias} maps to both {previous} and {primary}")
            alias_to_primary[alias] = primary

    graph: dict[str, set[str]] = {}
    unresolved: dict[str, set[str]] = {}
    edge_count = 0
    for path in sources:
        source = primary_module_name(path)
        imports = {
            import_module_name(raw)
            for raw in USE_RE.findall(path.read_text(encoding="utf-8", errors="replace"))
        }
        targets: set[str] = set()
        for imported in imports:
            if not imported.startswith("vitte/compiler"):
                continue
            target = alias_to_primary.get(imported)
            if target is None:
                unresolved.setdefault(imported, set()).add(source)
                continue
            targets.add(target)
        graph[source] = targets
        edge_count += len(targets)

    for imported, importers in sorted(unresolved.items()):
        failures.append(
            f"compiler import has no source module: {imported} "
            f"(from {', '.join(sorted(importers))})"
        )
    return graph, edge_count, alias_to_primary


def reachable_modules(graph: dict[str, set[str]]) -> set[str]:
    entry = primary_module_name(ENTRYPOINT)
    reached: set[str] = set()
    pending = deque([entry])
    while pending:
        current = pending.popleft()
        if current in reached:
            continue
        reached.add(current)
        pending.extend(graph.get(current, set()) - reached)
    return reached


def runtime_contract(reachable: set[str], failures: list[str]) -> dict[str, Any]:
    evidence: dict[str, Any] = {
        "compiler": rel(BOOTSTRAP),
        "entrypoint": rel(ENTRYPOINT),
        "expected_reachable_module_count": len(reachable),
    }
    if not BOOTSTRAP.is_file() or not os.access(BOOTSTRAP, os.X_OK):
        failures.append(f"missing executable bootstrap compiler: {rel(BOOTSTRAP)}")
        return evidence

    environment = os.environ.copy()
    for name in ("VITTE_ROOT", "VITTE_SYSROOT", "VITTE_HOME"):
        environment.pop(name, None)
    with tempfile.TemporaryDirectory(prefix="vitte-compiler-graph-cwd-") as temporary:
        command = [str(BOOTSTRAP), "check", str(ENTRYPOINT)]
        completed = subprocess.run(
            command,
            cwd=temporary,
            env=environment,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )
    evidence.update(
        {
            "command": [rel(BOOTSTRAP), "check", rel(ENTRYPOINT)],
            "exit_code": completed.returncode,
            "output_tail": completed.stdout[-6000:].replace(str(ROOT), "<repo>"),
            "compiled_reachable_module_count": len(reachable) if completed.returncode == 0 else 0,
            "cwd_outside_repository": True,
            "environment_root_variables_removed": True,
        }
    )
    if completed.returncode != 0:
        failures.append("bootstrap compiler failed to compile the complete entrypoint import closure")
    return evidence


def write_report(payload: dict[str, Any]) -> None:
    REPORT_JSON.parent.mkdir(parents=True, exist_ok=True)
    REPORT_JSON.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    runtime = payload["runtime"]
    lines = [
        "# Compiler Full Import Graph Gate",
        "",
        f"- status: {payload['status']}",
        f"- source_module_count: {payload['source_module_count']}",
        f"- graph_edge_count: {payload['graph_edge_count']}",
        f"- entrypoint_reachable_count: {payload['entrypoint_reachable_count']}",
        f"- compiled_reachable_module_count: {runtime.get('compiled_reachable_module_count', 0)}",
        f"- unreachable_source_count: {payload['unreachable_source_count']}",
        f"- compiler_exit_code: {runtime.get('exit_code', 'not-run')}",
        "",
        "## Failures",
        "",
    ]
    lines.extend(f"- {failure}" for failure in payload["failures"])
    if not payload["failures"]:
        lines.append("- none")
    REPORT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    failures: list[str] = []
    sources = compiler_sources()
    if len(sources) != EXPECTED_MODULES:
        failures.append(f"expected {EXPECTED_MODULES} compiler modules, found {len(sources)}")
    graph, edge_count, aliases = build_graph(sources, failures)
    reachable = reachable_modules(graph)
    runtime = runtime_contract(reachable, failures)
    if runtime.get("compiled_reachable_module_count") != len(reachable):
        failures.append(
            "compiled import closure coverage is "
            f"{runtime.get('compiled_reachable_module_count', 0)}/{len(reachable)}"
        )
    payload: dict[str, Any] = {
        "schema": "vitte.compiler-full-import-graph.v1",
        "source_root": rel(COMPILER_ROOT),
        "entrypoint": rel(ENTRYPOINT),
        "source_module_count": len(sources),
        "import_alias_count": len(aliases),
        "graph_edge_count": edge_count,
        "entrypoint_reachable_count": len(reachable),
        "unreachable_source_count": len(sources) - len(reachable),
        "runtime": runtime,
        "failures": failures,
    }
    payload["status"] = "pass" if not failures else "fail"
    write_report(payload)
    if failures:
        for failure in failures:
            print(f"[compiler-full-import-graph-gate][error] {failure}", file=sys.stderr)
        return 1
    print(
        "[compiler-full-import-graph-gate] ok: "
        f"sources={len(sources)} edges={edge_count} compiled_closure={len(reachable)}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
