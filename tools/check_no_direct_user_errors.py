#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
COMPILER_ROOT = ROOT / "src" / "vitte" / "compiler"
DRIVER = COMPILER_ROOT / "driver" / "compiler.vit"
ARCHITECTURE = COMPILER_ROOT / "diagnostics" / "architecture.vit"

ALLOWED_PRINT_PROCS = {
    "emit_diagnostic_output",
    "emit_cli_help",
    "emit_cli_success",
    "emit_serialized_output",
    "emit_internal_log",
}

ALLOWED_DIAGNOSTIC_OUTPUT_PROCS = {
    "emit_diagnostic_output",
    "print_frontend_diagnostics_list",
    "print_analysis_diagnostics_list",
    "print_user_diagnostic",
    "print_result_diagnostics",
}

REQUIRED_CONSTRUCTORS = (
    "compiler_diagnostic_error",
    "compiler_diagnostic_io_error",
    "compiler_diagnostic_cli_error",
    "compiler_diagnostic_backend_error",
    "compiler_diagnostic_linker_error",
)

REQUIRED_STAGES = (
    "Cli",
    "Io",
    "Lexer",
    "Parser",
    "Resolver",
    "Sema",
    "TypeChecker",
    "BorrowChecker",
    "Mir",
    "Ir",
    "Backend",
    "Linker",
)


def strip_vitte_strings_and_comments(line: str) -> str:
    line = re.sub(r'"(?:\\.|[^"\\])*"', '""', line)
    line = line.split("//", 1)[0]
    if line.lstrip().startswith("#"):
        return ""
    return line


def source_findings() -> list[str]:
    findings: list[str] = []
    for path in sorted(COMPILER_ROOT.rglob("*.vit")):
        current_proc = ""
        for line_number, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            code = strip_vitte_strings_and_comments(raw)
            proc_match = re.match(r"\s*proc\s+([A-Za-z_][A-Za-z0-9_]*)", code)
            if proc_match:
                current_proc = proc_match.group(1)

            if re.search(r"\bprint\s*\(", code) and current_proc not in ALLOWED_PRINT_PROCS:
                findings.append(
                    f"{path.relative_to(ROOT)}:{line_number}: direct print outside an approved output boundary ({current_proc or 'top-level'})"
                )

            if re.search(r"\bemit_diagnostic_output\s*\(", code) and current_proc not in ALLOWED_DIAGNOSTIC_OUTPUT_PROCS:
                findings.append(
                    f"{path.relative_to(ROOT)}:{line_number}: diagnostic output bypasses the canonical renderer ({current_proc or 'top-level'})"
                )

            for name in ("printf", "echo", "cli_error"):
                if re.search(rf"\b{name}\s*\(", code):
                    findings.append(
                        f"{path.relative_to(ROOT)}:{line_number}: direct {name} user-error surface is forbidden"
                    )

            if "[vitte][error]" in raw:
                findings.append(
                    f"{path.relative_to(ROOT)}:{line_number}: raw [vitte][error] marker is forbidden"
                )

    architecture = ARCHITECTURE.read_text(encoding="utf-8")
    for constructor in REQUIRED_CONSTRUCTORS:
        if not re.search(rf"\bproc\s+{re.escape(constructor)}\s*\(", architecture):
            findings.append(f"{ARCHITECTURE.relative_to(ROOT)}: missing canonical constructor {constructor}")
    if "proc compiler_diagnostic_render(" not in architecture:
        findings.append(f"{ARCHITECTURE.relative_to(ROOT)}: missing canonical text/JSON/LSP renderer")
    for stage in REQUIRED_STAGES:
        if f"  {stage}\n" not in architecture:
            findings.append(f"{ARCHITECTURE.relative_to(ROOT)}: missing diagnostic stage {stage}")
    for prefix in (
        "E_CLI_",
        "E_IO_",
        "LEX",
        "PARSE_",
        "MOD_",
        "RESOLVE_",
        "SEMA_",
        "TYPECK_",
        "BORROWCK_",
        "MIR_",
        "IR_",
        "BACKEND_",
        "LINK_",
    ):
        if f'starts_with(code, "{prefix}")' not in architecture:
            findings.append(f"{ARCHITECTURE.relative_to(ROOT)}: missing diagnostic routing for {prefix}")

    driver = DRIVER.read_text(encoding="utf-8")
    forbidden_driver_terms = (
        "report.diagnostic(",
        "report.render_text",
        "dump_diagnostics_lsp(",
        "append_source_contract_diagnostics(",
        "render_internal_compiler_error_text(",
        "[vitte][error]",
    )
    for term in forbidden_driver_terms:
        if term in driver:
            findings.append(f"{DRIVER.relative_to(ROOT)}: forbidden legacy diagnostic path {term}")
    for required in (
        'compiler_diagnostic_io_error("E_CLI_IO"',
        "print_user_diagnostic(request, diagnostic0)",
        "compiler_diagnostic_render(",
        'env_get("VITTE_LANG")',
        "lang: cli_environment_language()",
    ):
        if required not in driver:
            findings.append(f"{DRIVER.relative_to(ROOT)}: missing build diagnostic integration {required}")
    return findings


def runtime_findings() -> list[str]:
    findings: list[str] = []
    for relative in (
        Path("bin/vitte"),
        Path("bin/vittec"),
        Path("target/stage1/vitte"),
        Path("target/stage2/vitte"),
        Path("target/release/vitte"),
    ):
        path = ROOT / relative
        if not path.exists():
            findings.append(f"{relative}: runtime compiler artifact is missing")
            continue
        data = path.read_bytes()
        if b"[vitte][error]" in data:
            findings.append(f"{relative}: binary still embeds raw [vitte][error]")
        if b"E_CLI_IO: cannot read" in data:
            findings.append(f"{relative}: binary still embeds direct E_CLI_IO text")
    return findings


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--runtime", action="store_true", help="also inspect release compiler artifacts")
    args = parser.parse_args()

    findings = source_findings()
    if args.runtime:
        findings.extend(runtime_findings())
    if findings:
        for finding in findings:
            print(f"[no-direct-user-errors][error] {finding}", file=sys.stderr)
        print(f"[no-direct-user-errors] status=failed findings={len(findings)}", file=sys.stderr)
        return 1
    print("[no-direct-user-errors] status=ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
