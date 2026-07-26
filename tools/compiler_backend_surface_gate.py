#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "target/release/vitte"
REPORT_DIR = ROOT / "target/reports"
REPORT_JSON = REPORT_DIR / "compiler_backend_surface_gate.json"
REPORT_MD = REPORT_DIR / "compiler_backend_surface_gate.md"
FIXTURE_DIR = ROOT / "tests/backend_surface"
OUT = ROOT / "target/compiler-backend-surface-gate"


EVIDENCE: dict[str, list[tuple[str, str]]] = {
    "top_level_const_form_pick_proc_use_export": [
        ("src/vitte/compiler/middle/hir/pretty.vit", "HirItemKind.Use"),
        ("src/vitte/compiler/middle/hir/pretty.vit", "HirItemKind.Export"),
        ("src/vitte/compiler/middle/hir/pretty.vit", "HirItemKind.Const"),
        ("src/vitte/compiler/middle/hir/pretty.vit", "HirItemKind.Form"),
        ("src/vitte/compiler/middle/hir/pretty.vit", "HirItemKind.Pick"),
        ("src/vitte/compiler/middle/hir/pretty.vit", "HirItemKind.Proc"),
        ("src/vitte/compiler/middle/lower/mir_to_ir.vit", "item.kind == HirItemKind.Form"),
        ("src/vitte/compiler/middle/lower/mir_to_ir.vit", "item.kind == HirItemKind.Pick"),
    ],
    "compiler_generics": [
        ("src/vitte/compiler/middle/hir/validate.vit", "generic_count"),
        ("src/vitte/compiler/middle/lower/mir_to_ir.vit", "item.generic_count"),
        ("src/vitte/compiler/backend/c/emitter.vit", "generic_count"),
    ],
    "calls_blocks_if_while_match": [
        ("src/vitte/compiler/middle/lower/mir_to_ir.vit", "IrInstructionKind.Call"),
        ("src/vitte/compiler/backend/ir/ir.vit", "IrInstructionKind.Call"),
        ("src/vitte/compiler/backend/ir/ir.vit", "IrInstructionKind.Branch"),
        ("src/vitte/compiler/backend/ir/ir.vit", "IrInstructionKind.Switch"),
        ("src/vitte/compiler/middle/hir/pretty.vit", "HirExprKind.Block"),
        ("src/vitte/compiler/middle/hir/pretty.vit", "HirExprKind.If"),
        ("src/vitte/compiler/middle/hir/pretty.vit", "HirExprKind.Match"),
    ],
    "structs_forms": [
        ("src/vitte/compiler/middle/lower/mir_to_ir.vit", "IrInstructionKind.Aggregate"),
        ("src/vitte/compiler/backend/ir/ir.vit", "verify_aggregate_instruction"),
        ("src/vitte/compiler/backend/c/emitter.vit", "emit_nominal_declaration"),
    ],
    "arrays_lists": [
        ("src/vitte/compiler/middle/lower/hir_to_mir.vit", "MirRvalueKind.Array"),
        ("src/vitte/compiler/middle/lower/mir_to_ir.vit", "MirRvalueKind.Array"),
        ("src/vitte/compiler/middle/hir/pretty.vit", "HirExprKind.Array"),
    ],
    "strings": [
        ("src/vitte/compiler/middle/hir/pretty.vit", "HirExprKind.String"),
        ("src/vitte/compiler/middle/infer/constraints.vit", "InferTypeKind.String"),
        ("src/vitte/compiler/backend/c/architecture.vit", "c_type_for_vitte_type"),
    ],
    "errors_result": [
        ("src/vitte/compiler/backend/c/pipeline.vit", "CBackendPipelineResult"),
        ("src/vitte/compiler/backend/c/emitter.vit", "CEmitResult"),
        ("src/vitte/compiler/backend/native_bridge.vit", "NativeBridgeResult"),
    ],
    "modules_imports": [
        ("src/vitte/compiler/backend/ir/ir.vit", "IrModule"),
        ("src/vitte/compiler/middle/lower/mir_to_ir.vit", "module_add_external_symbol"),
        ("src/vitte/compiler/backend/link/artifact.vit", "imported"),
    ],
}


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def clean_env() -> dict[str, str]:
    env = dict(os.environ)
    env.pop("VITTE_ROOT", None)
    env["VITTE_COMPILER"] = str(BIN)
    env["VITTE_PACKAGE_OFFLINE"] = "1"
    return env


def run(args: list[str]) -> dict[str, Any]:
    proc = subprocess.run(
        [str(BIN), *args],
        cwd=ROOT,
        env=clean_env(),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    return {"args": args, "exit_code": proc.returncode, "stdout": proc.stdout.strip(), "stderr": proc.stderr.strip()}


def check_evidence() -> tuple[list[dict[str, Any]], list[str]]:
    rows: list[dict[str, Any]] = []
    failures: list[str] = []
    for category, needles in EVIDENCE.items():
        missing: list[str] = []
        for rel_path, needle in needles:
            path = ROOT / rel_path
            if not path.is_file() or needle not in path.read_text(encoding="utf-8"):
                missing.append(f"{rel_path}: {needle}")
        passed = not missing
        rows.append({"category": category, "passed": passed, "checks": len(needles), "missing": missing})
        if missing:
            failures.append(f"backend surface evidence missing for {category}: {', '.join(missing)}")
    return rows, failures


def check_fixtures() -> tuple[list[dict[str, Any]], list[str]]:
    OUT.mkdir(parents=True, exist_ok=True)
    commands: list[dict[str, Any]] = []
    failures: list[str] = []
    fixtures = sorted(FIXTURE_DIR.glob("*.vit"))
    if not fixtures:
        failures.append(f"missing backend surface fixtures: {rel(FIXTURE_DIR)}")
        return commands, failures
    for path in fixtures:
        source = rel(path)
        check = run(["check", source])
        output = OUT / path.with_suffix("").name
        build = run(["build", source, "-o", rel(output)])
        commands.extend([check, build])
        if check["exit_code"] != 0:
            failures.append(f"backend surface check failed: {source}")
        if build["exit_code"] != 0 or not output.is_file():
            failures.append(f"backend surface build failed: {source}")
    return commands, failures


def write_reports(status: str, evidence: list[dict[str, Any]], commands: list[dict[str, Any]], failures: list[str]) -> None:
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    payload = {
        "schema": "vitte.compiler.backend.surface.gate.v1",
        "status": status,
        "evidence": evidence,
        "fixtures": rel(FIXTURE_DIR),
        "commands": commands,
        "failures": failures,
    }
    REPORT_JSON.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    lines = [
        "# compiler backend surface gate",
        "",
        f"- status: {status}",
        f"- categories: {len(evidence)}",
        f"- commands: {len(commands)}",
    ]
    if failures:
        lines.append("")
        lines.append("## Failures")
        lines.extend(f"- {failure}" for failure in failures)
    REPORT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    failures: list[str] = []
    commands: list[dict[str, Any]] = []
    if not BIN.is_file():
        failures.append("missing target/release/vitte")
        evidence: list[dict[str, Any]] = []
    else:
        evidence, evidence_failures = check_evidence()
        fixture_commands, fixture_failures = check_fixtures()
        failures.extend(evidence_failures)
        failures.extend(fixture_failures)
        commands.extend(fixture_commands)
    status = "fail" if failures else "pass"
    write_reports(status, evidence, commands, failures)
    if failures:
        print("[compiler-backend-surface-gate][error] gate failed", file=sys.stderr)
        for failure in failures:
            print(f" - {failure}", file=sys.stderr)
        return 1
    print(f"[compiler-backend-surface-gate] ok categories={len(evidence)} commands={len(commands)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
