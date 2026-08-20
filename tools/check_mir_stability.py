#!/usr/bin/env python3
"""Check the canonical MIR data model, validation, and analysis contracts."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MIR_DIR = ROOT / "src/vitte/compiler/middle/mir"
MANIFEST = ROOT / "src/vitte/compiler/tests/mir_coverage_manifest.json"
REPORT_DIR = ROOT / "target/reports"
REPORT = REPORT_DIR / "mir_stability.json"
MARKDOWN = REPORT_DIR / "mir_stability.md"

REQUIRED_FILES = (
    "builder.vit",
    "dataflow.vit",
    "mir.vit",
    "mod.vit",
    "monomorphize.vit",
    "pretty.vit",
    "transform.vit",
    "validate.vit",
)

REQUIRED_CONTRACTS = {
    "mir.vit": (
        "pick MirStatementKind {",
        "pick MirRvalueKind {",
        "pick MirTerminatorKind {",
        "form MirPlace {",
        "form MirOperand {",
        "form MirRvalue {",
        "form MirBlock {",
        "form MirFunction {",
        "form MirUnit {",
        "proc mir_terminator_return_at",
        "proc mir_terminator_goto_at",
        "proc mir_terminator_branch_at",
    ),
    "validate.vit": (
        "proc block_exists",
        "proc rvalue_valid",
        "proc canonical_borrow_rvalue_valid",
        "proc terminator_valid_for_blocks",
        "proc function_valid",
        "proc mir_validation_reason",
        "proc validate_mir",
        "MirTerminatorKind.Goto",
        "MirTerminatorKind.Branch",
        "MirTerminatorKind.Switch",
    ),
    "dataflow.vit": (
        "pick DataflowDirection {",
        "pick DataflowStateKind {",
        "pick DataflowFactKind {",
        "form DataflowResult {",
        "proc analyze_function",
        "proc analyze_mir",
        "proc build_summary",
        "proc dataflow_selftest",
    ),
    "transform.vit": (
        "pick MirTransformKind {",
        "pick MirTransformResultKind {",
        "proc optimize_function",
        "proc optimize_module",
        "proc transform_report_for",
        "proc mir_transform_selftest",
        "MirTransformKind.Validate",
    ),
    "monomorphize.vit": (
        "form MirGenericInstance {",
        "proc generic_instance_symbol",
        "proc collect_function_instances",
        "proc instantiate_function",
        "proc monomorphize_mir",
        "MIR_MONOMORPHIZATION_LIMIT",
    ),
    "pretty.vit": (
        "proc pretty_rvalue",
        "proc pretty_statement",
        "proc pretty_terminator",
        "proc pretty_block",
        "proc pretty_function",
        "proc pretty_unit",
        "proc mir_pretty_selftest",
    ),
    "builder.vit": (
        "pick MirBuilderPhase {",
        "form MirBuilderContext {",
        "proc allocate_block",
        "proc allocate_value",
        "proc mark_terminated",
        "proc builder_valid",
        "proc mir_builder_selftest",
    ),
    "mod.vit": (
        "use vitte/compiler/middle/mir/validate",
        "use vitte/compiler/middle/mir/monomorphize",
        "export *",
    ),
}


def main() -> int:
    failures: list[str] = []
    module_text: dict[str, str] = {}
    for name in REQUIRED_FILES:
        path = MIR_DIR / name
        if not path.is_file():
            failures.append(f"missing canonical MIR module: {name}")
            continue
        module_text[name] = path.read_text(encoding="utf-8")

    contract_count = 0
    for name, needles in REQUIRED_CONTRACTS.items():
        text = module_text.get(name, "")
        for needle in needles:
            contract_count += 1
            if needle not in text:
                failures.append(f"{name}: missing contract `{needle}`")

    manifest_surfaces = 0
    if not MANIFEST.is_file():
        failures.append("missing MIR coverage manifest")
    else:
        payload = json.loads(MANIFEST.read_text(encoding="utf-8"))
        entries = payload.get("entries", [])
        manifest_surfaces = len(entries) if isinstance(entries, list) else 0
        if manifest_surfaces == 0:
            failures.append("MIR coverage manifest has no entries")
        if not re.search(r"mir\.MirUnit", "\n".join(str(entry) for entry in entries)):
            failures.append("MIR coverage manifest does not cover MirUnit")

    report = {
        "schema": "vitte.compiler.mir-stability",
        "status": "error" if failures else "ok",
        "module_count": len(module_text),
        "contract_count": contract_count,
        "manifest_surface_count": manifest_surfaces,
        "failures": failures,
    }
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    MARKDOWN.write_text(
        "# MIR stability\n\n"
        f"- modules: {report['module_count']}\n"
        f"- contracts: {report['contract_count']}\n"
        f"- manifest surfaces: {report['manifest_surface_count']}\n"
        f"- status: {report['status']}\n",
        encoding="utf-8",
    )
    if failures:
        for failure in failures:
            print(f"[mir-stability][error] {failure}", file=sys.stderr)
        return 1
    print(
        "[mir-stability] "
        f"modules={report['module_count']} contracts={report['contract_count']} "
        f"manifest_surfaces={report['manifest_surface_count']} status=ok"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
