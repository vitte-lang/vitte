#!/usr/bin/env python3
"""Check the executable Vitte-native backend path and compile its source graph."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BOOTSTRAP = ROOT / "target" / "bootstrap-c17" / "vitte-bootstrap"
REPORT = ROOT / "target" / "reports" / "native_vitte_backend_stability.json"


SURFACES: dict[str, tuple[str, ...]] = {
    "src/vitte/compiler/backend/api.vit": (
        "proc backend_api_version()",
        'give "canonical"',
        "compile_to_valid_ir_with_profile_and_packaging",
        "emit_native_object_from_assembly",
        "link_native_executable",
    ),
    "src/vitte/compiler/backend/pipeline.vit": (
        "run_codegen_x86_64_with_profile",
        "link_ir_unit_with_kind",
        'backend_name == "llvm" or backend_name == "llvm-ir"',
    ),
    "src/vitte/compiler/backend/codegen/mod.vit": (
        "select_ir_unit_x86_64_with_debug",
        "allocate_virtual_locations",
        "emit_x86_64_text",
        'object_format: "elf64-relocatable"',
    ),
    "src/vitte/compiler/backend/native_bridge.vit": (
        "host_emit_assembly_object",
        "host_verify_native_object",
        "host_link_executable",
        "proc emit_native_object_from_assembly(",
    ),
    "src/vitte/compiler/backend/native_toolchain.vit": (
        "form NativeToolchain {",
        "form NativeToolCommand {",
        "proc plan_assembly_object_command(",
        "proc plan_link_executable_command(",
        'give "x86_64-unknown-linux-gnu"',
    ),
    "src/vitte/compiler/driver/compile.vit": (
        "compile_to_valid_ir_with_profile_and_packaging",
        "emit_native_object_from_assembly(backend0.codegen.assembly",
        "link_native_executable(object0.object_result.object_path",
        "native_output_overwrites_source",
    ),
}


FORBIDDEN_MARKERS: dict[str, tuple[str, ...]] = {
    "src/vitte/compiler/backend/pipeline.vit": (
        "vitte_stage0_clone_self",
        "vitte-bootstrap-payload-bridge",
        ".bootstrap-bridge",
    ),
    "src/vitte/compiler/backend/native_bridge.vit": (
        "host_system(",
        "cp bin/vitte",
        "target/stage1/vitte",
    ),
    "src/vitte/compiler/driver/compile.vit": (
        "vitte_stage0_clone_self",
        "vitte-bootstrap-payload-bridge",
        ".bootstrap-bridge",
    ),
}


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def source_checks() -> tuple[list[dict[str, object]], list[str]]:
    rows: list[dict[str, object]] = []
    failures: list[str] = []
    for relative, needles in SURFACES.items():
        path = ROOT / relative
        text = path.read_text(encoding="utf-8") if path.is_file() else ""
        missing = [needle for needle in needles if needle not in text]
        rows.append({"surface": relative, "checked": len(needles), "missing": missing})
        if missing:
            failures.append(f"{relative}: missing {', '.join(missing)}")

    for relative, markers in FORBIDDEN_MARKERS.items():
        path = ROOT / relative
        text = path.read_text(encoding="utf-8") if path.is_file() else ""
        present = [marker for marker in markers if marker in text]
        rows.append({"surface": relative, "forbidden_checked": len(markers), "present": present})
        if present:
            failures.append(f"{relative}: forbidden native-copy marker {', '.join(present)}")
    return rows, failures


def source_graph() -> list[Path]:
    paths = [
        ROOT / "src/vitte/compiler/backend/api.vit",
        ROOT / "src/vitte/compiler/backend/pipeline.vit",
        ROOT / "src/vitte/compiler/backend/native_bridge.vit",
        ROOT / "src/vitte/compiler/backend/native_toolchain.vit",
        ROOT / "src/vitte/compiler/backend/diagnostics.vit",
        ROOT / "src/vitte/compiler/backend/verified_pipeline.vit",
        ROOT / "src/vitte/compiler/driver/compile.vit",
    ]
    paths.extend(sorted((ROOT / "src/vitte/compiler/backend/codegen").glob("*.vit")))
    paths.extend(sorted((ROOT / "src/vitte/compiler/backend/ir").glob("*.vit")))
    paths.extend(sorted((ROOT / "src/vitte/compiler/backend/link").glob("*.vit")))
    paths.extend(sorted((ROOT / "src/vitte/compiler/backend/target").glob("*.vit")))
    paths.extend(sorted((ROOT / "src/vitte/compiler/backends/vitte_emit").glob("*.vit")))
    return list(dict.fromkeys(paths))


def compile_graph() -> tuple[list[dict[str, object]], list[str]]:
    rows: list[dict[str, object]] = []
    failures: list[str] = []
    if not BOOTSTRAP.is_file():
        return [], [f"missing bootstrap compiler: {rel(BOOTSTRAP)}"]
    for path in source_graph():
        proc = subprocess.run(
            [str(BOOTSTRAP), "check", rel(path)],
            cwd=ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )
        output = proc.stdout.strip()
        rows.append({"file": rel(path), "exit_code": proc.returncode})
        if proc.returncode != 0:
            failures.append(f"{rel(path)}: bootstrap check failed\n{output}")
    return rows, failures


def main() -> int:
    source_rows, failures = source_checks()
    compile_rows, compile_failures = compile_graph()
    failures.extend(compile_failures)
    payload = {
        "schema": "vitte.compiler.native_vitte_backend_stability",
        "schema_version": "1.0.0",
        "status": "fail" if failures else "pass",
        "source_checks": source_rows,
        "compiled_files": compile_rows,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(
        f"[native-vitte-backend] status={payload['status']} "
        f"source_checks={len(source_rows)} compiled={len(compile_rows)}"
    )
    for failure in failures:
        print(f"[native-vitte-backend][error] {failure}", file=sys.stderr)
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
