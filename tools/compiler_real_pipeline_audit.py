#!/usr/bin/env python3
from __future__ import annotations

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "target" / "reports" / "compiler_real_pipeline" / "audit.json"


def read(rel: str) -> str:
    return (ROOT / rel).read_text(encoding="utf-8", errors="replace")


def has(text: str, needle: str) -> bool:
    return needle in text


def check_required_steps() -> list[dict[str, str]]:
    compile_vit = read("src/vitte/compiler/driver/compile.vit")
    lower_ast_vit = read("src/vitte/compiler/middle/hir/lower_ast.vit")
    typeck_api_vit = read("src/vitte/compiler/analysis/typeck/api.vit")
    backend_vit = read("src/vitte/compiler/backend/pipeline.vit")
    codegen_vit = read("src/vitte/compiler/backend/codegen/mod.vit")
    linker_vit = read("src/vitte/compiler/backend/link/linker.vit")

    checks = [
        ("source", "driver/compile", "source_text", has(compile_vit, "source_text")),
        ("lexer", "frontend pipeline", "frontend_run", has(compile_vit, "frontend_run")),
        ("parser", "frontend pipeline", "frontend_run", has(compile_vit, "frontend_run")),
        ("ast", "frontend output", "FrontendOutput", has(compile_vit, "FrontendOutput")),
        ("hir", "AST to HIR lowering", "lower_ast_to_hir", has(compile_vit, "lower_ast_to_hir")),
        (
            "hir_validate",
            "validated AST to HIR lowering",
            "lower_ast_to_hir -> validate_hir(lower_ast_to_hir_unvalidated)",
            has(compile_vit, "lower_ast_to_hir")
            and has(lower_ast_vit, "give validate_hir(lower_ast_to_hir_unvalidated(frontend));"),
        ),
        ("sema", "semantic analysis", "run_sema_hir", has(compile_vit, "run_sema_hir")),
        (
            "typeck",
            "canonical HIR type checking API",
            "run_production_typeck_hir -> run_typeck_hir",
            has(compile_vit, "run_production_typeck_hir")
            and has(typeck_api_vit, 'give "canonical";')
            and has(typeck_api_vit, "give run_typeck_hir(hir);"),
        ),
        ("borrowck", "analysis result", "borrowck_ok", has(compile_vit, "borrowck_ok")),
        ("mir", "HIR to MIR lowering", "lower_hir_to_mir", has(compile_vit, "lower_hir_to_mir")),
        ("mir_validate", "MIR validation", "validate_mir", has(compile_vit, "validate_mir")),
        ("ir", "MIR to IR lowering", "lower_mir_as_ir_module", has(compile_vit, "lower_mir_as_ir_module")),
        (
            "backend_pipeline",
            "backend pipeline",
            "compile_to_valid_ir_with_profile_and_packaging",
            has(backend_vit, "compile_to_valid_ir_with_profile_and_packaging"),
        ),
        ("codegen", "code generation", "run_codegen_x86_64", has(codegen_vit, "run_codegen_x86_64")),
        ("object", "object emission", "object_text", has(codegen_vit, "object_text")),
        ("linker", "linker", "link_ir_unit_with_kind", has(linker_vit, "link_ir_unit_with_kind")),
        ("binary_output", "link artifact output", "output_create", has(linker_vit, "output_create")),
    ]

    out: list[dict[str, str]] = []
    for name, owner, needle, present in checks:
        out.append(
            {
                "name": name,
                "owner": owner,
                "needle": needle,
                "status": "present" if present else "missing",
            }
        )
    return out


def detect_cli_entry() -> dict[str, str]:
    compiler = read("src/vitte/compiler/main.vit")
    config = read("toolchain/bootstrap-config.json")
    seed_root_ok = '"compiler": "toolchain/seed/vittec0.seed"' in config
    entry_ok = 'const COMPILER_ENTRY_POINT: string = "src/vitte/compiler/main.vit"' in compiler
    main_placeholder = re.search(
        r"proc\s+main\s*\(\s*args:\s*list\[string\]\s*\)\s*->\s*int\s*\{\s*give\s+0\s*;?\s*\}",
        compiler,
        re.S,
    ) is not None
    return {
        "trust_root": "toolchain/seed/vittec0.seed" if seed_root_ok else "unknown",
        "compiler_entry_point": "src/vitte/compiler/main.vit" if entry_ok else "unknown",
        "source_entry_declared": "vitte/compiler/main",
        "runtime_cli_dispatch": "placeholder" if main_placeholder else "wired",
        "status": "real-entry-with-placeholder-main" if seed_root_ok and entry_ok and main_placeholder else "ok",
    }


def detect_forbidden_surfaces() -> list[dict[str, str]]:
    files = {
        "src/vitte/compiler/backend/codegen/mod.vit": read("src/vitte/compiler/backend/codegen/mod.vit"),
        "src/vitte/compiler/backend/codegen/object.vit": read("src/vitte/compiler/backend/codegen/object.vit"),
        "src/vitte/compiler/backend/link/linker.vit": read("src/vitte/compiler/backend/link/linker.vit"),
    }
    patterns = [
        ("pseudo-object", "pseudo object text is not a native object file"),
        ("elf-pseudo", "pseudo ELF format is not a machine object"),
        ("vitte-bootstrap-artifact", "bootstrap artifact name marks linker output as adapter-level"),
        ("build_native_wrapper", "native wrapper bridge is compatibility, not real backend emission"),
        ("native bridge: wrapping stage artifact", "stage2 bridge wraps shell payload instead of linking backend object"),
        ("build-native output remains the v1-compatible shell artifact", "seed compiler cannot yet build the real native stage2 driver"),
    ]

    found: list[dict[str, str]] = []
    for path, text in files.items():
        for pattern, reason in patterns:
            if pattern in text:
                found.append({"file": path, "pattern": pattern, "reason": reason})
    return found


def detect_informational_markers() -> list[dict[str, str]]:
    seed = read("toolchain/seed/vittec0.seed")
    notes = [
        (
            "build-native output remains the v1-compatible shell artifact",
            "bootstrap seed still documents the bridge-era native wrapper behavior",
        ),
        (
            "vitte-bootstrap-native-bridge",
            "bootstrap seed still contains bridge marker support for stage artifacts",
        ),
    ]

    found: list[dict[str, str]] = []
    for pattern, reason in notes:
        if pattern in seed:
            found.append({"file": "toolchain/seed/vittec0.seed", "pattern": pattern, "reason": reason})
    return found


def detect_pipeline_bypasses() -> list[dict[str, str]]:
    compiler_root = ROOT / "src" / "vitte" / "compiler"
    allowed_unvalidated_hir = {
        "src/vitte/compiler/middle/hir/lower_ast.vit",
    }
    allowed_direct_typeck = {
        "src/vitte/compiler/analysis/typeck/api.vit",
        "src/vitte/compiler/analysis/typeck/checker.vit",
    }
    bypasses: list[dict[str, str]] = []

    for path in sorted(compiler_root.rglob("*.vit")):
        rel = path.relative_to(ROOT).as_posix()
        text = path.read_text(encoding="utf-8", errors="replace")
        if "lower_ast_to_hir_unvalidated" in text and rel not in allowed_unvalidated_hir:
            bypasses.append(
                {
                    "file": rel,
                    "pattern": "lower_ast_to_hir_unvalidated",
                    "reason": "production HIR consumers must use validated lowering",
                }
            )
        if "run_typeck_hir" in text and rel not in allowed_direct_typeck:
            bypasses.append(
                {
                    "file": rel,
                    "pattern": "run_typeck_hir",
                    "reason": "production type checking must use run_production_typeck_hir",
                }
            )

    return bypasses


def detect_bridge_artifacts() -> tuple[list[dict[str, str]], list[dict[str, str]]]:
    candidates = [
        ROOT / "target" / "bootstrap" / "stage2" / "vittec",
        ROOT / "bin" / "vitte",
        ROOT / "bin" / "vittec",
    ]
    forbidden: list[dict[str, str]] = []
    informational: list[dict[str, str]] = []
    for path in candidates:
        sidecar = Path(str(path) + ".bootstrap-bridge")
        if sidecar.is_file():
            source = ""
            try:
                for line in sidecar.read_text(encoding="utf-8", errors="replace").splitlines():
                    if line.startswith("src="):
                        source = line[4:]
                        break
            except OSError:
                source = ""
            reason = "native stage artifact is still a bootstrap bridge wrapper"
            if source:
                reason = reason + f" for {source}"
            item = {
                "file": str(sidecar.relative_to(ROOT)),
                "pattern": "vitte-bootstrap-native-bridge",
                "reason": reason,
            }
            if source in (str(ROOT / "src/vitte/compiler/main.vit"), "src/vitte/compiler/main.vit"):
                informational.append(item)
            else:
                forbidden.append(item)
    return forbidden, informational


def main() -> int:
    steps = check_required_steps()
    cli_entry = detect_cli_entry()
    bridge_forbidden, bridge_informational = detect_bridge_artifacts()
    forbidden = detect_forbidden_surfaces() + bridge_forbidden
    informational = detect_informational_markers() + bridge_informational
    bypasses = detect_pipeline_bypasses()
    missing_steps = [s for s in steps if s["status"] != "present"]

    failures: list[str] = []
    if missing_steps:
        failures.append("pipeline step missing from source")
    if cli_entry["runtime_cli_dispatch"] == "placeholder":
        failures.append("CLI source entry has placeholder main(args) and is not the real command dispatcher")
    if forbidden:
        failures.append("non-real backend/link/stage2 surface detected")
    if bypasses:
        failures.append("canonical pipeline bypass detected")

    status = "fail" if failures else "pass"
    payload = {
        "schema": "vitte.compiler.real_pipeline_audit",
        "schema_version": "1.0.0",
        "status": status,
        "cli_entry": cli_entry,
        "pipeline_steps": steps,
        "forbidden_surfaces": forbidden,
        "pipeline_bypasses": bypasses,
        "informational_markers": informational,
        "failures": failures,
        "expected_flow": [
            "source .vit",
            "lexer",
            "parser",
            "AST",
            "HIR",
            "HIR validation",
            "sema",
            "typeck",
            "borrowck",
            "MIR",
            "MIR validation/optimisation",
            "IR",
            "backend codegen",
            "object file",
            "linker",
            "native binary",
        ],
    }

    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, ensure_ascii=True, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    print(f"[compiler-real-pipeline] status={status} report={REPORT.relative_to(ROOT)}")
    print(
        "[compiler-real-pipeline] cli_entry="
        f"{cli_entry['compiler_entry_point']} runtime_dispatch={cli_entry['runtime_cli_dispatch']}"
    )
    for step in steps:
        print(f"[compiler-real-pipeline][step] {step['name']} {step['status']} owner={step['owner']}")
    for item in forbidden:
        print(f"[compiler-real-pipeline][forbid] {item['file']} pattern={item['pattern']} reason={item['reason']}")
    for item in bypasses:
        print(f"[compiler-real-pipeline][bypass] {item['file']} pattern={item['pattern']} reason={item['reason']}")
    for item in informational:
        print(f"[compiler-real-pipeline][note] {item['file']} pattern={item['pattern']} reason={item['reason']}")
    for failure in failures:
        print(f"[compiler-real-pipeline][error] {failure}")

    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
