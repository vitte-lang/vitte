#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
RESOLVER_ROOT = ROOT / "src/vitte/compiler/analysis/resolution"
BOOTSTRAP = ROOT / "target/bootstrap-c17/vitte-bootstrap"
IMPORT_SOURCE = ROOT / "bootstrap/src/import/import.c"
DRIVER_SOURCE = ROOT / "bootstrap/src/driver/driver.c"
REPORT_JSON = ROOT / "target/reports/resolver_root_independence_gate.json"
REPORT_MD = ROOT / "target/reports/resolver_root_independence_gate.md"

FORBIDDEN_RESOLVER_TEXT = (
    "VITTE_ROOT",
    "getcwd(",
    "chdir(",
    "/Users/",
    "Documents/Github",
    "src/vitte/",
)


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def static_contract(failures: list[str]) -> dict[str, Any]:
    resolver_sources = sorted(RESOLVER_ROOT.glob("*.vit"))
    forbidden_hits: list[dict[str, str]] = []
    for path in resolver_sources:
        text = path.read_text(encoding="utf-8", errors="replace")
        for marker in FORBIDDEN_RESOLVER_TEXT:
            if marker in text:
                forbidden_hits.append({"file": rel(path), "marker": marker})

    import_text = IMPORT_SOURCE.read_text(encoding="utf-8", errors="replace")
    driver_text = DRIVER_SOURCE.read_text(encoding="utf-8", errors="replace")
    checks = {
        "resolver_sources_present": len(resolver_sources) > 0,
        "resolver_sources_have_no_repository_root_dependency": not forbidden_hits,
        "import_resolver_defines_ancestor_resolution": (
            "static bool vitte_import_resolve_ancestor_paths(" in import_text
        ),
        "import_resolver_calls_ancestor_resolution": (
            "vitte_import_resolve_ancestor_paths(request, &module_path, &resolved_path)" in import_text
        ),
        "driver_has_no_cwd_src_probe": 'vitte_fs_is_directory("src")' not in driver_text,
        "driver_has_no_cwd_src_search_path": (
            'vitte_import_resolver_add_search_path(resolver, "src")' not in driver_text
        ),
    }
    for name, passed in checks.items():
        if not passed:
            failures.append(f"static resolver-root contract failed: {name}")
    for hit in forbidden_hits:
        failures.append(f"forbidden root dependency {hit['marker']!r} in {hit['file']}")
    return {
        "checks": checks,
        "resolver_source_count": len(resolver_sources),
        "forbidden_hits": forbidden_hits,
    }


def runtime_contract(failures: list[str]) -> dict[str, Any]:
    evidence: dict[str, Any] = {"compiler": rel(BOOTSTRAP)}
    if not BOOTSTRAP.is_file() or not os.access(BOOTSTRAP, os.X_OK):
        failures.append(f"missing executable bootstrap compiler: {rel(BOOTSTRAP)}")
        return evidence

    with tempfile.TemporaryDirectory(prefix="vitte-resolver-root-") as temporary:
        temporary_root = Path(temporary)
        relocated_source = temporary_root / "relocated/src"
        relocated_resolver = relocated_source / "vitte/compiler/analysis/resolution"
        relocated_probe = relocated_source / "checks/resolver_root_probe.vit"
        unrelated_cwd = temporary_root / "cwd"
        relocated_resolver.mkdir(parents=True)
        relocated_probe.parent.mkdir(parents=True)
        unrelated_cwd.mkdir()
        (relocated_resolver / "root_probe_target.vit").write_text(
            "space vitte/compiler/analysis/resolution/root_probe_target\n\n"
            "export *\n\n"
            "proc resolver_root_probe_value() -> int {\n"
            "    give 0;\n"
            "}\n",
            encoding="utf-8",
        )
        relocated_probe.write_text(
            "space checks/resolver_root_probe\n\n"
            "use vitte/compiler/analysis/resolution/root_probe_target.{\n"
            "    resolver_root_probe_value\n"
            "}\n\n"
            "proc main() -> int {\n"
            "    give resolver_root_probe_value();\n"
            "}\n",
            encoding="utf-8",
        )
        command = [str(BOOTSTRAP), "check", str(relocated_probe)]
        env = os.environ.copy()
        for name in ("VITTE_ROOT", "VITTE_SYSROOT", "VITTE_HOME"):
            env.pop(name, None)
        completed = subprocess.run(
            command,
            cwd=unrelated_cwd,
            env=env,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )
        evidence.update(
            {
                "command": command,
                "cwd_is_outside_relocated_tree": True,
                "environment_root_variables_removed": True,
                "exit_code": completed.returncode,
                "output_tail": completed.stdout[-6000:],
                "relocated_probe": str(relocated_probe),
                "canonical_import": "vitte/compiler/analysis/resolution/root_probe_target",
            }
        )
        if completed.returncode != 0:
            failures.append("relocated resolver failed from an unrelated working directory")
    return evidence


def write_report(payload: dict[str, Any]) -> None:
    REPORT_JSON.parent.mkdir(parents=True, exist_ok=True)
    REPORT_JSON.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    runtime = payload["runtime"]
    lines = [
        "# Resolver Root Independence Gate",
        "",
        f"- status: {payload['status']}",
        f"- resolver_source_count: {payload['static']['resolver_source_count']}",
        f"- compiler: {runtime.get('compiler', 'missing')}",
        f"- relocated_exit_code: {runtime.get('exit_code', 'not-run')}",
        f"- cwd_is_outside_relocated_tree: {runtime.get('cwd_is_outside_relocated_tree', False)}",
        f"- environment_root_variables_removed: {runtime.get('environment_root_variables_removed', False)}",
        "",
        "## Failures",
        "",
    ]
    failures = payload["failures"]
    lines.extend(f"- {failure}" for failure in failures)
    if not failures:
        lines.append("- none")
    REPORT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    failures: list[str] = []
    payload: dict[str, Any] = {
        "schema": "vitte.resolver-root-independence.v1",
        "static": static_contract(failures),
        "runtime": runtime_contract(failures),
        "failures": failures,
    }
    payload["status"] = "pass" if not failures else "fail"
    write_report(payload)
    if failures:
        for failure in failures:
            print(f"[resolver-root-independence-gate][error] {failure}", file=sys.stderr)
        return 1
    print(
        "[resolver-root-independence-gate] ok: "
        f"{payload['static']['resolver_source_count']} resolver sources; relocated check passed"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
