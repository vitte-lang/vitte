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
    backend_vit = read("src/vitte/compiler/backend/pipeline.vit")
    codegen_vit = read("src/vitte/compiler/backend/codegen/mod.vit")
    linker_vit = read("src/vitte/compiler/backend/link/linker.vit")

    checks = [
        ("source", "driver/compile", compile_vit, "source_text"),
        ("lexer", "frontend pipeline", compile_vit, "frontend_run"),
        ("parser", "frontend pipeline", compile_vit, "frontend_run"),
        ("ast", "frontend output", compile_vit, "FrontendOutput"),
        ("hir", "AST to HIR lowering", compile_vit, "lower_ast_to_hir"),
        ("hir_validate", "HIR validation", compile_vit, "validate_hir"),
        ("sema", "semantic analysis", compile_vit, "run_sema_hir"),
        ("typeck", "type checking", compile_vit, "run_typeck_hir"),
        ("borrowck", "analysis result", compile_vit, "borrowck_ok"),
        ("mir", "HIR to MIR lowering", compile_vit, "lower_hir_to_mir"),
        ("mir_validate", "MIR validation", compile_vit, "validate_mir"),
        ("ir", "MIR to IR lowering", compile_vit, "lower_mir_as_ir_module"),
        ("backend_pipeline", "backend pipeline", backend_vit, "compile_to_valid_ir_with_profile_and_packaging"),
        ("codegen", "code generation", codegen_vit, "run_codegen_x86_64"),
        ("object", "object emission", codegen_vit, "object_text"),
        ("linker", "linker", linker_vit, "link_ir_unit_with_kind"),
        ("binary_output", "link artifact output", linker_vit, "output_create"),
    ]

    out: list[dict[str, str]] = []
    for name, owner, text, needle in checks:
        out.append(
            {
                "name": name,
                "owner": owner,
                "needle": needle,
                "status": "present" if has(text, needle) else "missing",
            }
        )
    return out


def detect_cli_entry() -> dict[str, str]:
    stage2 = read("toolchain/scripts/bootstrap/stage2.sh")
    compiler = read("src/vitte/compiler/main.vit")
    source_root_ok = 'COMPILER_SOURCE_ROOT="$ROOT_DIR/src/vitte/compiler"' in stage2
    entry_ok = 'COMPILER_ENTRY_POINT="$COMPILER_SOURCE_ROOT/main.vit"' in stage2
    main_placeholder = re.search(
        r"proc\s+main\s*\(\s*args:\s*list\[string\]\s*\)\s*->\s*int\s*\{\s*give\s+0\s*;?\s*\}",
        compiler,
        re.S,
    ) is not None
    return {
        "stage2_source_root": "src/vitte/compiler" if source_root_ok else "unknown",
        "stage2_entry_point": "src/vitte/compiler/main.vit" if entry_ok else "unknown",
        "source_entry_declared": "vitte/compiler/main",
        "runtime_cli_dispatch": "placeholder" if main_placeholder else "wired",
        "status": "real-entry-with-placeholder-main" if source_root_ok and entry_ok and main_placeholder else "ok",
    }


def detect_forbidden_surfaces() -> list[dict[str, str]]:
    files = {
        "src/vitte/compiler/backend/codegen/mod.vit": read("src/vitte/compiler/backend/codegen/mod.vit"),
        "src/vitte/compiler/backend/codegen/object.vit": read("src/vitte/compiler/backend/codegen/object.vit"),
        "src/vitte/compiler/backend/link/linker.vit": read("src/vitte/compiler/backend/link/linker.vit"),
        "toolchain/scripts/bootstrap/stage2.sh": read("toolchain/scripts/bootstrap/stage2.sh"),
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
    missing_steps = [s for s in steps if s["status"] != "present"]

    failures: list[str] = []
    if missing_steps:
        failures.append("pipeline step missing from source")
    if cli_entry["runtime_cli_dispatch"] == "placeholder":
        failures.append("CLI source entry has placeholder main(args) and is not the real command dispatcher")
    if forbidden:
        failures.append("non-real backend/link/stage2 surface detected")

    status = "fail" if failures else "pass"
    payload = {
        "schema": "vitte.compiler.real_pipeline_audit",
        "schema_version": "1.0.0",
        "status": status,
        "cli_entry": cli_entry,
        "pipeline_steps": steps,
        "forbidden_surfaces": forbidden,
        "informational_markers": informational,
        "failures": failures,
        "expected_flow": [
            "source .vit",
            "lexer",
            "parser",
            "AST",
            "HIR",
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
        f"{cli_entry['stage2_entry_point']} runtime_dispatch={cli_entry['runtime_cli_dispatch']}"
    )
    for step in steps:
        print(f"[compiler-real-pipeline][step] {step['name']} {step['status']} owner={step['owner']}")
    for item in forbidden:
        print(f"[compiler-real-pipeline][forbid] {item['file']} pattern={item['pattern']} reason={item['reason']}")
    for item in informational:
        print(f"[compiler-real-pipeline][note] {item['file']} pattern={item['pattern']} reason={item['reason']}")
    for failure in failures:
        print(f"[compiler-real-pipeline][error] {failure}")

    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
