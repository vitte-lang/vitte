#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DIAGNOSTIC = ROOT / "src/vitte/compiler/diagnostics/diagnostic.vit"
ARCHITECTURE = ROOT / "src/vitte/compiler/diagnostics/architecture.vit"
DRIVER = ROOT / "src/vitte/compiler/driver/compiler.vit"
CATALOG = ROOT / "src/vitte/compiler/infrastructure/diagnostics/fluent_catalog.vit"
EN = ROOT / "locales/en/diagnostics.ftl"
FR = ROOT / "locales/fr/diagnostics.ftl"
EN_EXPLAIN = ROOT / "locales/en/diagnostics_explain.ftl"
FR_EXPLAIN = ROOT / "locales/fr/diagnostics_explain.ftl"

CODE = "TYPECK_E_ASSIGN_MISMATCH"
EN_MESSAGE = "assignment type mismatch"
FR_MESSAGE = "affectation type incompatibilite"


def parse_ftl(path: Path) -> dict[str, str]:
    entries: dict[str, str] = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        entries[key.strip()] = value.strip()
    return entries


def fail(errors: list[str]) -> int:
    print("[cli-diagnostics-fluent-source][error] Fluent diagnostic source contract failed", file=sys.stderr)
    for error in errors:
        print(f"  - {error}", file=sys.stderr)
    return 1


def main() -> int:
    errors: list[str] = []
    diagnostic = DIAGNOSTIC.read_text(encoding="utf-8")
    architecture = ARCHITECTURE.read_text(encoding="utf-8")
    driver = DRIVER.read_text(encoding="utf-8")
    catalog = CATALOG.read_text(encoding="utf-8")
    en = parse_ftl(EN)
    fr = parse_ftl(FR)
    en_explain = parse_ftl(EN_EXPLAIN)
    fr_explain = parse_ftl(FR_EXPLAIN)

    if en.get(CODE) != EN_MESSAGE:
        errors.append(f"{EN.relative_to(ROOT)} must define {CODE} = {EN_MESSAGE!r}")
    if fr.get(CODE) != FR_MESSAGE:
        errors.append(f"{FR.relative_to(ROOT)} must define {CODE} = {FR_MESSAGE!r}")
    for code in ("E_CLI_IO", "E_CLI_MISSING_ARG", "E_CLI_UNKNOWN_COMMAND", "E_CLI_UNKNOWN_OPTION", "E_CLI_INVALID_ARGUMENT"):
        for suffix in ("summary", "cause", "step1", "fix", "example"):
            key = f"{code}.{suffix}"
            if not en_explain.get(key):
                errors.append(f"{EN_EXPLAIN.relative_to(ROOT)} missing rich Fluent field {key}")
            if not fr_explain.get(key):
                errors.append(f"{FR_EXPLAIN.relative_to(ROOT)} missing rich Fluent field {key}")

    required_diagnostic_terms = (
        "FluentDiagnosticFields",
        "proc diagnostic_localized(diagnostic: Diagnostic, lang: string)",
        "fluent_catalog_normalize_locale(lang)",
        "fluent_catalog_fields_or(locale, diagnostic.code, diagnostic.message)",
        "set localized.message = fields.message",
        "set localized.root_cause = fields.cause",
        "set localized.probable_fix = fields.fix_it",
        "set localized.valid_example = fields.corrected_example",
    )
    for term in required_diagnostic_terms:
        if term not in diagnostic:
            errors.append(f"{DIAGNOSTIC.relative_to(ROOT)} missing {term!r}")

    required_architecture_terms = (
        "proc compiler_diagnostic_localized_report(report: DiagnosticReport, lang: string)",
        "proc compiler_diagnostic_surface(report: DiagnosticReport, lang: string, command: string, source: string)",
        "proc compiler_diagnostic_render(report: DiagnosticReport, format: string, lang: string, command: string, source: string)",
        "render_terminal_report(localized",
        "compiler_diagnostic_json(command, source, localized)",
        "lsp_publish_diagnostics_json_rpc(uri, localized)",
    )
    for term in required_architecture_terms:
        if term not in architecture:
            errors.append(f"{ARCHITECTURE.relative_to(ROOT)} missing {term!r}")

    if "const FLUENT_DEFAULT_LOCALE: string = \"en\";" not in catalog:
        errors.append("generated Fluent catalog must default to en")
    if "if locale == \"\" {\n        give FLUENT_DEFAULT_LOCALE;" not in catalog:
        errors.append("generated Fluent catalog must fallback empty locale to en")
    if "if locale == \"fr\" or locale == \"fr-FR\"" not in catalog:
        errors.append("generated Fluent catalog must normalize fr aliases")

    signature = "proc print_result_diagnostics(result0: CompilerResult, request: CompilerCliRequest)"
    if signature not in driver:
        errors.append(f"{DRIVER.relative_to(ROOT)} must carry the complete diagnostic request")
    for term in ("request.error_format", "request.lang", "compiler_diagnostic_render(", 'env_get("VITTE_LANG")'):
        if term not in driver:
            errors.append(f"driver canonical diagnostic rendering missing {term!r}")
    for term in (
        "output_explicit: bool",
        "no_native: bool",
        "output_path: \"\"",
        "output_explicit: false",
        "no_native: false",
        "set request.output_explicit = true",
        "set request.no_native = true",
        "if not request.output_explicit or request.output_path == \"\"",
        "build requires an explicit output path with -o",
        "if request.no_native",
        "LINK_E_OUTPUT_NOT_MATERIALIZED",
    ):
        if term not in driver:
            errors.append(f"driver build output contract missing {term!r}")

    build_contract_terms = (
        'if request.command == "build"',
        'compiler_diagnostic_io_error("E_IO_OVERWRITE_FORBIDDEN"',
        "let validation_result: CompilerResult = run_compiler(validate_compiler)",
        "if validation_result.status != driver.DriverExitStatus::Ok",
        "if request.no_native",
        "let compiler0: Compiler = compiler_from_input(source_path, source_text, virtual_source, opts)",
    )
    overwrite_offset = driver.find(build_contract_terms[1])
    build_start = driver.rfind(build_contract_terms[0], 0, overwrite_offset)
    build_contract_offsets = [driver.find(term, build_start) for term in build_contract_terms]
    if any(offset < 0 for offset in build_contract_offsets):
        errors.append("driver build flow is missing overwrite, validation, no-native, or codegen contract terms")
    elif build_contract_offsets != sorted(build_contract_offsets):
        errors.append("driver build flow must reject overwrite, validate source, handle --no-native, then run codegen")

    call_sites = re.findall(r"print_result_diagnostics\(([^)]*)\)", driver)
    non_signature_calls = [site for site in call_sites if "CompilerResult" not in site]
    bad_calls = [site for site in non_signature_calls if not re.search(r",\s*request\s*$", site)]
    if bad_calls:
        errors.append("all driver diagnostic print call sites must pass the complete request")
        errors.extend(f"bad call: print_result_diagnostics({site})" for site in bad_calls)
    if len(non_signature_calls) < 4:
        errors.append("expected check/build/run/test diagnostic print call sites")

    if errors:
        return fail(errors)

    print("[cli-diagnostics-fluent-source] status=ok default_locale=en localized_runtime_path=source")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
