#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DIAGNOSTIC = ROOT / "src/vitte/compiler/diagnostics/diagnostic.vit"
ARCHITECTURE = ROOT / "src/vitte/compiler/diagnostics/architecture.vit"
RENDER = ROOT / "src/vitte/compiler/diagnostics/render.vit"
JSON_RENDERER = ROOT / "src/vitte/compiler/diagnostics/json.vit"
LSP_RENDERER = ROOT / "src/vitte/compiler/diagnostics/lsp.vit"
DRIVER = ROOT / "src/vitte/compiler/driver/compiler.vit"
TYPED_CATALOG = ROOT / "src/vitte/compiler/diagnostics/catalog.vit"
CATALOG = ROOT / "src/vitte/compiler/infrastructure/diagnostics/fluent_catalog.vit"
CATALOG_SOURCES = (CATALOG, *sorted(CATALOG.parent.glob("fluent_catalog_*.vit")))
EN = ROOT / "locales/en/diagnostics.ftl"
FR = ROOT / "locales/fr/diagnostics.ftl"
EN_EXPLAIN = ROOT / "locales/en/diagnostics_explain.ftl"
FR_EXPLAIN = ROOT / "locales/fr/diagnostics_explain.ftl"

CODE = "TYPECK_E_ASSIGN_MISMATCH"
EN_MESSAGE = "assignment type mismatch"
FR_MESSAGE = "affectation type incompatibilite"
PUBLIC_BUILD_CODES = (
    "E_CLI_MISSING_ARG",
    "E_CLI_UNKNOWN_COMMAND",
    "E_CLI_UNKNOWN_OPTION",
    "E_CLI_INVALID_ARGUMENT",
    "E_IO_FILE_NOT_FOUND",
    "E_IO_FILE_UNREADABLE",
    "E_IO_OUTPUT_UNWRITABLE",
    "E_IO_PERMISSION_DENIED",
    "E_IO_DIRECTORY_NOT_FOUND",
    "E_IO_OVERWRITE_FORBIDDEN",
    "BACKEND_E_MISSING_C_COMPILER",
    "BACKEND_E_UNSUPPORTED_TARGET",
    "LINK_E_OUTPUT_NOT_MATERIALIZED",
    "LINK_E_UNDEFINED_SYMBOL",
    "LINK_E_COMMAND_FAILED",
)


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
    render = RENDER.read_text(encoding="utf-8")
    json_renderer = JSON_RENDERER.read_text(encoding="utf-8")
    lsp_renderer = LSP_RENDERER.read_text(encoding="utf-8")
    driver = DRIVER.read_text(encoding="utf-8")
    typed_catalog = TYPED_CATALOG.read_text(encoding="utf-8")
    catalog = "\n".join(path.read_text(encoding="utf-8") for path in CATALOG_SOURCES)
    en = parse_ftl(EN)
    fr = parse_ftl(FR)
    en_explain = parse_ftl(EN_EXPLAIN)
    fr_explain = parse_ftl(FR_EXPLAIN)

    if en.get(CODE) != EN_MESSAGE:
        errors.append(f"{EN.relative_to(ROOT)} must define {CODE} = {EN_MESSAGE!r}")
    if fr.get(CODE) != FR_MESSAGE:
        errors.append(f"{FR.relative_to(ROOT)} must define {CODE} = {FR_MESSAGE!r}")
    for code in PUBLIC_BUILD_CODES:
        if not en.get(code):
            errors.append(f"{EN.relative_to(ROOT)} missing public build code {code}")
        if not fr.get(code):
            errors.append(f"{FR.relative_to(ROOT)} missing public build code {code}")
        if f'diagnostic_code("{code}"' not in typed_catalog:
            errors.append(f"{TYPED_CATALOG.relative_to(ROOT)} missing public build code {code}")
        if f'if code == "{code}"' not in catalog:
            errors.append(f"{CATALOG.relative_to(ROOT)} missing public build code {code}")
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
        "if diagnostic0.severity == DiagnosticSeverity.Fatal",
        "set diagnostic0.severity = DiagnosticSeverity.Error",
        'valid: text != "" and json != "" and lsp != ""',
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
    if 'locale == "pt-BR"' not in catalog or 'locale == "pt_BR"' not in catalog:
        errors.append("generated Fluent catalog must normalize pt-BR and pt_BR aliases")
    if 'locale == "zh-CN"' not in catalog or 'locale == "zh_CN"' not in catalog:
        errors.append("generated Fluent catalog must normalize zh-CN and zh_CN aliases")
    if "give FLUENT_DEFAULT_LOCALE;\n}" not in catalog:
        errors.append("generated Fluent catalog must fallback unknown locales to en")

    signature = "proc print_result_diagnostics(result0: CompilerResult, request: CompilerCliRequest)"
    if signature not in driver:
        errors.append(f"{DRIVER.relative_to(ROOT)} must carry the complete diagnostic request")
    for term in ("request.error_format", "request.lang", "compiler_diagnostic_render(", 'env_get("VITTE_LANG")'):
        if term not in driver:
            errors.append(f"driver canonical diagnostic rendering missing {term!r}")

    prescan_signature = "proc cli_prescan_diagnostic_context(args: list[string], request: CompilerCliRequest)"
    parse_signature = "proc parse_cli_request(args: list[string])"
    prescan_call = "cli_prescan_diagnostic_context(args, empty_cli_request())"
    command_validation = 'if request.command == ""'
    for term in (
        prescan_signature,
        prescan_call,
        'arg == "--diagnostics-json"',
        'arg == "--diagnostics-lsp"',
        'arg == "--lang"',
        'starts_with_text(arg, "--lang=")',
        'if arg == "--"',
    ):
        if term not in driver:
            errors.append(f"driver early diagnostic context missing {term!r}")
    parse_offset = driver.find(parse_signature)
    prescan_call_offset = driver.find(prescan_call, parse_offset)
    command_validation_offset = driver.find(command_validation, parse_offset)
    if parse_offset < 0 or prescan_call_offset < 0 or command_validation_offset < 0:
        errors.append("driver early diagnostic context ordering cannot be verified")
    elif not (parse_offset < prescan_call_offset < command_validation_offset):
        errors.append("driver must pre-scan diagnostic format and language before validating the command")

    environment_offset = driver.find('env_get("VITTE_LANG")')
    cli_language_offset = driver.find('set request.lang = args[i + 1]', driver.find(prescan_signature))
    if environment_offset < 0 or cli_language_offset < 0 or environment_offset > cli_language_offset:
        errors.append("driver locale precedence must initialize VITTE_LANG before applying --lang")

    for term in (
        'if not options_ended and arg == "--"',
        '"unexpected positional argument: " + arg',
        'starts_with_text(arg, "--color=")',
        '"unsupported --counterfactual-level value: "',
        'compiler_diagnostic_cli_error("E_CLI_UNKNOWN_COMMAND", "unknown pkg subcommand: "',
        'args[index + 1] != ""',
        '"--native and --no-native cannot be used together"',
    ):
        if term not in driver:
            errors.append(f"driver stable CLI parsing contract missing {term!r}")
    if "set request.error_format = cli_normalize_error_format" in driver:
        errors.append("driver must validate a diagnostic format before replacing the pre-scanned JSON/LSP format")

    direct_prints = re.findall(r"(?m)^\s*print\(", driver)
    if len(direct_prints) != 1:
        errors.append(f"driver must have exactly one direct print primitive, found {len(direct_prints)}")
    if 'compiler_diagnostic_io_error("E_CLI_IO"' in driver:
        errors.append("driver must not emit the legacy generic E_CLI_IO code")
    for emitter in ("emit_cli_help", "emit_cli_success", "emit_serialized_output", "emit_internal_log"):
        match = re.search(rf"proc {emitter}\(text: string\) \{{(.*?)\n\}}", driver, re.S)
        if match is None or "emit_diagnostic_output(text);" not in match.group(1):
            errors.append(f"{emitter} must delegate to emit_diagnostic_output")

    invalid_request_branch = re.search(
        r"if not request\.valid \{(.*?)give cli_exit_code\(request\.status\);",
        driver,
        re.S,
    )
    if invalid_request_branch is None or "print_user_diagnostic(request, diagnostic0);" not in invalid_request_branch.group(1):
        errors.append("all invalid CLI requests must render through print_user_diagnostic")
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
        'compiler_diagnostic_io_error("E_IO_FILE_NOT_FOUND"',
        'compiler_diagnostic_io_error("E_IO_FILE_UNREADABLE"',
        'compiler_diagnostic_io_error("E_IO_DIRECTORY_NOT_FOUND"',
        'compiler_diagnostic_io_error("E_IO_OUTPUT_UNWRITABLE"',
        'read_file(request.output_path) == source_text',
        "proc compiler_public_backend_code(code: string)",
        'give "BACKEND_E_MISSING_C_COMPILER"',
        'give "BACKEND_E_UNSUPPORTED_TARGET"',
        'give "LINK_E_UNDEFINED_SYMBOL"',
        'give "LINK_E_COMMAND_FAILED"',
        'give "E_IO_OUTPUT_UNWRITABLE"',
        'give "E_IO_PERMISSION_DENIED"',
        "compiler_output_span(compiler, compiler.options.output_path)",
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

    if driver.count("and not cli_uses_serialized_diagnostics(request)") < 8:
        errors.append("all check/build/run trace and timing output must be suppressed for JSON/LSP diagnostics")

    for term in (
        '"  = id: "',
        '"  = category: "',
        '"  = severity: "',
        '"  = fluent-key: "',
        '"  = span: "',
        '"  = label: "',
        '"  = cause: "',
        '"  = help: "',
        '"  = fix-it: "',
        '"  = corrected example: "',
    ):
        if term not in render:
            errors.append(f"terminal renderer missing rich field {term}")

    for term in ('ch == "\\""', 'ch == "\\n"', 'ch == "\\r"', 'ch == "\\t"', 'ch == "\\u0008"', 'ch == "\\u000c"'):
        if term not in json_renderer:
            errors.append(f"JSON renderer missing escape contract {term!r}")
    for term in ('proc json_control_escape(', 'ch == "\\u0000"', 'ch == "\\u001f"', 'json_control_escape(ch)'):
        if term not in json_renderer:
            errors.append(f"JSON renderer missing strict control-character escape contract {term!r}")

    for term in (
        'json_key_value("codeDescription"',
        'json_key_value("range"',
        'json_key_value("relatedInformation"',
        'json_key_value("message", json_quote(diagnostic.message))',
        'json_key_value("span", json_source_span(diagnostic.span))',
        'json_key_value("labels", json_labels(diagnostic.labels))',
        'json_key_value("suggestions", json_suggestions(diagnostic.suggestions))',
        'json_key_value("jsonrpc", json_quote("2.0"))',
        'json_key_value("method", json_quote("textDocument/publishDiagnostics"))',
    ):
        if term not in lsp_renderer:
            errors.append(f"LSP renderer missing rich contract {term!r}")

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
