#!/usr/bin/env python3
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "target" / "reports" / "backend_ir_contract_audit.json"
CANONICAL_IR = "src/vitte/compiler/backend/ir/ir.vit"


def read(rel: str) -> str:
    path = ROOT / rel
    if not path.is_file():
        print(f"[backend-ir-contract][error] missing file: {rel}", file=sys.stderr)
        raise SystemExit(1)
    return path.read_text(encoding="utf-8", errors="replace")


def compiler_sources() -> list[Path]:
    return sorted((ROOT / "src" / "vitte" / "compiler").rglob("*.vit"))


def production_sources() -> list[Path]:
    out: list[Path] = []
    for path in compiler_sources():
        rel = path.relative_to(ROOT).as_posix()
        if rel.startswith("src/vitte/compiler/tests/"):
            continue
        if rel.startswith("src/vitte/compiler/ir/"):
            continue
        out.append(path)
    return out


def add_check(
    results: list[dict[str, str]],
    failures: list[str],
    rel: str,
    needle: str,
    reason: str,
    present: bool,
) -> None:
    results.append(
        {
            "file": rel,
            "needle": needle,
            "reason": reason,
            "status": "present" if present else "missing",
        }
    )
    if not present:
        failures.append(f"{rel}: missing `{needle}`")


def main() -> int:
    required: list[tuple[str, str, str]] = [
        (CANONICAL_IR, 'const BACKEND_IR_DIALECT: string = "vitte.backend.ir"', "canonical dialect must be named"),
        (CANONICAL_IR, "const BACKEND_IR_SCHEMA_VERSION: u64 = 1", "canonical schema must be versioned"),
        (CANONICAL_IR, "dialect: string", "every unit must carry its dialect"),
        (CANONICAL_IR, "schema_version: u64", "every unit must carry its schema version"),
        (CANONICAL_IR, "proc unit_contract_is_supported(", "consumer compatibility check must exist"),
        (CANONICAL_IR, 'give "unsupported-ir-contract";', "verification must reject incompatible units"),
        ("src/vitte/compiler/middle/lower/mir_to_ir.vit", "proc lower_mir_to_ir_for_target(", "MIR lowering must own backend IR construction"),
        ("src/vitte/compiler/middle/pipeline.vit", "ir: IrUnit", "middle pipeline must expose the canonical unit"),
        ("src/vitte/compiler/backend/codegen/mod.vit", "unit: IrUnit", "codegen must consume the canonical unit"),
        ("src/vitte/compiler/backend/c/lowering.vit", "ir_unit: IrUnit", "C lowering must consume the canonical unit"),
        ("src/vitte/compiler/backends/llvm_bindings/mod.vit", "unit: IrUnit", "LLVM bindings must consume the canonical unit"),
        ("src/vitte/compiler/backend/link/linker.vit", "unit: IrUnit", "linking must consume the canonical unit"),
        ("src/vitte/compiler/tests/codegen_tests.vit", "test_backend_ir_contract_is_versioned_and_enforced", "contract compatibility must stay tested"),
        ("src/vitte/compiler/backend/ir/README.md", "only IR contract accepted", "canonical ownership must stay documented"),
    ]

    results: list[dict[str, str]] = []
    failures: list[str] = []
    for rel, needle, reason in required:
        add_check(results, failures, rel, needle, reason, needle in read(rel))

    source_texts = {
        path.relative_to(ROOT).as_posix(): path.read_text(encoding="utf-8", errors="replace")
        for path in compiler_sources()
    }
    canonical_definitions = sorted(
        rel for rel, source in source_texts.items() if "form IrUnit {" in source
    )
    definition_ok = canonical_definitions == [CANONICAL_IR]
    results.append(
        {
            "file": "src/vitte/compiler/**/*.vit",
            "needle": "form IrUnit {",
            "reason": "IrUnit must have exactly one canonical definition",
            "status": "canonical-only" if definition_ok else "duplicate",
        }
    )
    if not definition_ok:
        failures.append(f"IrUnit definitions are not canonical-only: {canonical_definitions}")

    direct_literals: list[str] = []
    for rel, source in source_texts.items():
        if rel == CANONICAL_IR:
            continue
        for line_number, line in enumerate(source.splitlines(), start=1):
            if "IrUnit {" in line and not re.search(r"->\s*IrUnit\s*\{", line):
                direct_literals.append(f"{rel}:{line_number}")
    literals_ok = not direct_literals
    results.append(
        {
            "file": "src/vitte/compiler/**/*.vit",
            "needle": "IrUnit form literal outside backend/ir/ir.vit",
            "reason": "all units must be created by canonical constructors",
            "status": "absent" if literals_ok else "forbidden-present",
        }
    )
    if direct_literals:
        failures.append("direct IrUnit literals outside canonical module: " + ", ".join(direct_literals))

    legacy_imports: list[str] = []
    legacy_import_pattern = re.compile(r"^use\s+vitte/compiler/ir(?:[./]|$)", re.MULTILINE)
    for path in production_sources():
        rel = path.relative_to(ROOT).as_posix()
        source = path.read_text(encoding="utf-8", errors="replace")
        if legacy_import_pattern.search(source):
            legacy_imports.append(rel)
    results.append(
        {
            "file": "src/vitte/compiler/{main,driver,frontend,analysis,middle,backend,backends}/**/*.vit",
            "needle": "use vitte/compiler/ir",
            "reason": "production code must not import legacy bootstrap IR fixtures",
            "status": "absent" if not legacy_imports else "forbidden-present",
        }
    )
    if legacy_imports:
        failures.append("legacy compiler/ir imports in production: " + ", ".join(legacy_imports))

    retired = [
        "src/vitte/compiler/backend/ir/block.vit",
        "src/vitte/compiler/backend/ir/function.vit",
        "src/vitte/compiler/backend/ir/instruction.vit",
        "src/vitte/compiler/backend/ir/module.vit",
    ]
    present_retired = [rel for rel in retired if (ROOT / rel).exists()]
    results.append(
        {
            "file": "src/vitte/compiler/backend/ir",
            "needle": "retired shadow IR modules",
            "reason": "obsolete shadow helpers must not redefine the canonical dialect",
            "status": "absent" if not present_retired else "forbidden-present",
        }
    )
    if present_retired:
        failures.append("retired shadow IR modules present: " + ", ".join(present_retired))

    payload = {
        "schema": "vitte.compiler.backend_ir_contract_audit",
        "schema_version": "1.0.0",
        "status": "fail" if failures else "pass",
        "canonical_ir": CANONICAL_IR,
        "dialect": "vitte.backend.ir",
        "ir_schema_version": 1,
        "results": results,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(
        json.dumps(payload, ensure_ascii=True, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )

    print(f"[backend-ir-contract] status={payload['status']} report={REPORT.relative_to(ROOT)}")
    for result in results:
        print(
            f"[backend-ir-contract][check] {result['status']} "
            f"{result['file']} needle={result['needle']}"
        )
    for failure in failures:
        print(f"[backend-ir-contract][error] {failure}")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
