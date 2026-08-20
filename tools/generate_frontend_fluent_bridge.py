#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "src" / "vitte" / "compiler" / "infrastructure" / "diagnostics" / "fluent_catalog.vit"
OUT_DIR = OUT.parent
CORE_CODES = ROOT / "tests" / "diag_snapshots" / "core_diagnostic_codes.txt"
RUNTIME_LOCALES = ("en", "fr", "es")
sys.path.insert(0, str(ROOT / "tools"))
from diagnostics_locales import SUPPORTED_DIAGNOSTIC_LOCALES, supported_locale_codes
from diagnostic_catalog_data import public_diagnostic_codes


def parse_ftl(path: Path) -> dict[str, str]:
    data: dict[str, str] = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        data[key.strip()] = value.strip()
    return data


def core_codes() -> list[str]:
    return public_diagnostic_codes([
        line.strip()
        for line in CORE_CODES.read_text(encoding="utf-8").splitlines()
        if line.strip() and not line.strip().startswith("#")
    ])


def vitte_escape(text: str) -> str:
    return text.replace("\\", "\\\\").replace('"', '\\"')


def symbol_for_locale(locale: str) -> str:
    return locale.replace("-", "_")


def render() -> str:
    locale_maps = {
        locale: parse_ftl(ROOT / "locales" / locale / "diagnostics.ftl")
        for locale in supported_locale_codes()
    }
    explain_maps = {
        locale: parse_ftl(ROOT / "locales" / locale / "diagnostics_explain.ftl")
        for locale in supported_locale_codes()
    }
    codes = core_codes()
    missing = [
        f"{locale}:{code}"
        for locale, data in locale_maps.items()
        for code in codes
        if code not in data
    ]
    if missing:
        preview = ", ".join(missing[:20])
        more = "" if len(missing) <= 20 else f" ... (+{len(missing) - 20} more)"
        raise RuntimeError(f"missing Fluent diagnostic keys: {preview}{more}")
    missing_explain = [
        f"{locale}:{code}.{field}"
        for locale, data in explain_maps.items()
        for code in codes
        for field in ("summary", "cause", "step1", "fix", "example")
        if f"{code}.{field}" not in data
    ]
    if missing_explain:
        preview = ", ".join(missing_explain[:20])
        more = "" if len(missing_explain) <= 20 else f" ... (+{len(missing_explain) - 20} more)"
        raise RuntimeError(f"missing Fluent diagnostic explanation keys: {preview}{more}")
    lines = [
        "space vitte/compiler/infrastructure/diagnostics/fluent_catalog",
        "",
        "export *",
        "",
        "form FluentCatalogEntry {",
        "    locale: string",
        "    code: string",
        "    message: string",
        "}",
        "",
        "form FluentCatalogStats {",
        "    locale_count: u64",
        "    diagnostic_count: u64",
        "    generated: bool",
        "}",
        "",
        "form FluentDiagnosticFields {",
        "    message: string",
        "    primary_label: string",
        "    secondary_label: string",
        "    cause: string",
        "    help: string",
        "    fix_it: string",
        "    corrected_example: string",
        "    note: string",
        "    lsp_code_action_title: string",
        "    valid: bool",
        "}",
        "",
        'const FLUENT_CATALOG_GENERATOR: string = "tools/generate_frontend_fluent_bridge.py";',
        'const FLUENT_DEFAULT_LOCALE: string = "en";',
        "",
        "proc fluent_catalog_entry(locale: string, code: string, message: string) -> FluentCatalogEntry {",
        "    give FluentCatalogEntry {",
        "        locale: locale,",
        "        code: code,",
        "        message: message",
        "    };",
        "}",
        "",
        "proc fluent_catalog_stats(locale_count: u64, diagnostic_count: u64, generated: bool) -> FluentCatalogStats {",
        "    give FluentCatalogStats {",
        "        locale_count: locale_count,",
        "        diagnostic_count: diagnostic_count,",
        "        generated: generated",
        "    };",
        "}",
        "",
        "proc fluent_diagnostic_fields(message: string, primary_label: string, secondary_label: string, cause: string, help: string, fix_it: string, corrected_example: string, note: string, lsp_code_action_title: string) -> FluentDiagnosticFields {",
        "    give FluentDiagnosticFields {",
        "        message: message,",
        "        primary_label: primary_label,",
        "        secondary_label: secondary_label,",
        "        cause: cause,",
        "        help: help,",
        "        fix_it: fix_it,",
        "        corrected_example: corrected_example,",
        "        note: note,",
        "        lsp_code_action_title: lsp_code_action_title,",
        "        valid: message != \"\" and primary_label != \"\" and cause != \"\" and help != \"\"",
        "    };",
        "}",
        "",
        "proc fluent_diagnostic_fields_empty() -> FluentDiagnosticFields {",
        "    give fluent_diagnostic_fields(\"\", \"\", \"\", \"\", \"\", \"\", \"\", \"\", \"\");",
        "}",
        "",
        "proc fluent_catalog_generator() -> string {",
        "    give FLUENT_CATALOG_GENERATOR;",
        "}",
        "",
        "proc fluent_default_locale() -> string {",
        "    give FLUENT_DEFAULT_LOCALE;",
        "}",
        "",
        "proc fluent_catalog_generated() -> bool {",
        "    give true;",
        "}",
        "",
        "proc fluent_catalog_locale_supported(locale: string) -> bool {",
    ]
    for index, locale in enumerate(supported_locale_codes()):
        prefix = "    give " if index == 0 else "        or "
        suffix = ";" if index == len(supported_locale_codes()) - 1 else ""
        lines.append(f'{prefix}locale == "{locale}"{suffix}')
    lines.extend([
        "}",
        "",
        "proc fluent_catalog_normalize_locale(locale: string) -> string {",
        "    if locale == \"\" {",
        "        give FLUENT_DEFAULT_LOCALE;",
        "    }",
        "",
    ])
    for locale in SUPPORTED_DIAGNOSTIC_LOCALES:
        aliases = (locale.code, *locale.aliases)
        condition = " or ".join(f'locale == "{alias}"' for alias in aliases)
        lines.extend([
            f"    if {condition} {{",
            f'        give "{locale.code}";',
            "    }",
            "",
        ])
    lines.extend([
        "    give FLUENT_DEFAULT_LOCALE;",
        "}",
        "",
    ])
    for locale in supported_locale_codes():
        fn_suffix = symbol_for_locale(locale)
        lines.append(f"proc fluent_catalog_fields_{fn_suffix}(code: string) -> FluentDiagnosticFields {{")
        for code in codes:
            message = locale_maps[locale].get(code, "")
            lines.append(f'    if code == "{code}" {{')
            lines.append(f'        give fluent_compact_fields("{vitte_escape(message)}");')
            lines.append("    }")
            lines.append("")
        lines.append("    give fluent_diagnostic_fields_empty();")
        lines.append("}")
        lines.append("")
    lines.extend([
        "proc fluent_catalog_fields(locale: string, code: string) -> FluentDiagnosticFields {",
        "    let normalized: string = fluent_catalog_normalize_locale(locale);",
        "",
    ])
    for locale in RUNTIME_LOCALES:
        lines.extend([
            f'    if normalized == "{locale}" {{',
            f"        give fluent_catalog_fields_{symbol_for_locale(locale)}(code);",
            "    }",
            "",
        ])
    lines.extend([
        "    give fluent_catalog_fields_en(code);",
        "}",
        "",
        "proc fluent_catalog_lookup(locale: string, code: string) -> string {",
        "    give fluent_catalog_fields(locale, code).message;",
        "}",
        "",
        "proc fluent_catalog_has(locale: string, code: string) -> bool {",
        "    give fluent_catalog_lookup(locale, code) != \"\";",
        "}",
        "",
        "proc fluent_catalog_entry_for(locale: string, code: string) -> FluentCatalogEntry {",
        "    let normalized: string = fluent_catalog_normalize_locale(locale);",
        "    give fluent_catalog_entry(normalized, code, fluent_catalog_lookup(normalized, code));",
        "}",
        "",
        "proc fluent_catalog_message_or(locale: string, code: string, fallback: string) -> string {",
        "    let message: string = fluent_catalog_lookup(locale, code);",
        "    if message == \"\" {",
        "        give fallback;",
        "    }",
        "    give message;",
        "}",
        "",
        "proc fluent_catalog_fields_or(locale: string, code: string, fallback: string) -> FluentDiagnosticFields {",
        "    let fields: FluentDiagnosticFields = fluent_catalog_fields(locale, code);",
        "    if fields.valid {",
        "        give fields;",
        "    }",
        "    give fluent_diagnostic_fields(",
        "        fallback,",
        "        fallback,",
        "        fallback,",
        "        \"the compiler rejected this operation\",",
        "        \"inspect the reported span and command arguments\",",
        "        \"repair the reported compiler contract\",",
        "        \"vitte check src/main.vit\",",
        "        fallback,",
        "        \"repair the reported compiler contract\"",
        "    );",
        "}",
        "",
        "proc fluent_catalog_stats_default() -> FluentCatalogStats {",
        f"    give fluent_catalog_stats({len(supported_locale_codes())}, {len(codes)}, true);",
        "}",
        "",
        "proc fluent_catalog_selftest() -> bool {",
        '    let en: string = fluent_catalog_lookup("en", "P0001");',
        '    let fr: string = fluent_catalog_lookup("fr-FR", "P0001");',
        '    let es: string = fluent_catalog_lookup("es-MX", "P0001");',
        '    let stats: FluentCatalogStats = fluent_catalog_stats_default();',
        "",
        "    give en != \"\"",
        "        and fr != \"\"",
        "        and es != \"\"",
        f"        and stats.locale_count == {len(supported_locale_codes())}",
        f"        and stats.diagnostic_count == {len(codes)}",
        "        and stats.generated",
        "        and fluent_catalog_has(\"zh_CN\", \"LEX_E_INVALID_CHAR\")",
        "        and fluent_catalog_generator() == \"tools/generate_frontend_fluent_bridge.py\";",
        "}",
        "",
    ])
    return "\n".join(lines)


def render_split() -> dict[Path, str]:
    generated = render().splitlines()
    locale_codes = supported_locale_codes()
    block_starts = {
        locale: next(
            index
            for index, line in enumerate(generated)
            if line.startswith(f"proc fluent_catalog_fields_{symbol_for_locale(locale)}(")
        )
        for locale in locale_codes
    }
    dispatcher_start = next(
        index
        for index, line in enumerate(generated)
        if line.startswith("proc fluent_catalog_fields(locale:")
    )
    first_locale_start = min(block_starts.values())
    main_lines = generated[:first_locale_start] + generated[dispatcher_start:]
    import_lines = [
        f"use vitte/compiler/infrastructure/diagnostics/fluent_catalog_{symbol_for_locale(locale)}.{{ fluent_catalog_fields_{symbol_for_locale(locale)} }}"
        for locale in RUNTIME_LOCALES
    ]
    export_index = main_lines.index("export *") + 1
    main_lines[export_index:export_index] = ["", *import_lines]

    outputs: dict[Path, str] = {
        OUT: "\n".join(main_lines).rstrip() + "\n"
    }
    for locale in locale_codes:
        start = block_starts[locale]
        following = [index for index in block_starts.values() if index > start]
        end = min(following + [dispatcher_start])
        suffix = symbol_for_locale(locale)
        locale_lines = [
            f"space vitte/compiler/infrastructure/diagnostics/fluent_catalog_{suffix}",
            "",
            "export *",
            "",
            "form FluentDiagnosticFields {",
            "    message: string",
            "    primary_label: string",
            "    secondary_label: string",
            "    cause: string",
            "    help: string",
            "    fix_it: string",
            "    corrected_example: string",
            "    note: string",
            "    lsp_code_action_title: string",
            "    valid: bool",
            "}",
            "",
            "proc fluent_diagnostic_fields(message: string, primary_label: string, secondary_label: string, cause: string, help: string, fix_it: string, corrected_example: string, note: string, lsp_code_action_title: string) -> FluentDiagnosticFields {",
            "    give FluentDiagnosticFields {",
            "        message: message,",
            "        primary_label: primary_label,",
            "        secondary_label: secondary_label,",
            "        cause: cause,",
            "        help: help,",
            "        fix_it: fix_it,",
            "        corrected_example: corrected_example,",
            "        note: note,",
            "        lsp_code_action_title: lsp_code_action_title,",
            "        valid: message != \"\" and primary_label != \"\" and cause != \"\" and help != \"\"",
            "    };",
            "}",
            "",
            "proc fluent_diagnostic_fields_empty() -> FluentDiagnosticFields {",
            "    give fluent_diagnostic_fields(\"\", \"\", \"\", \"\", \"\", \"\", \"\", \"\", \"\");",
            "}",
            "",
            "proc fluent_compact_fields(message: string) -> FluentDiagnosticFields {",
            "    if message == \"\" {",
            "        give fluent_diagnostic_fields_empty();",
            "    }",
            "    give fluent_diagnostic_fields(",
            "        message,",
            "        message,",
            "        message,",
            "        \"the compiler rejected this operation\",",
            "        \"inspect the reported span and command arguments\",",
            "        \"repair the reported compiler contract\",",
            "        \"vitte check src/main.vit\",",
            "        message,",
            "        \"repair the reported compiler contract\"",
            "    );",
            "}",
            "",
            *generated[start:end],
        ]
        outputs[OUT_DIR / f"fluent_catalog_{suffix}.vit"] = "\n".join(locale_lines).rstrip() + "\n"
    return outputs


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate the Vitte Fluent diagnostic catalog")
    parser.add_argument("--check", action="store_true", help="fail when the generated catalog is stale")
    args = parser.parse_args()
    generated = render_split()
    if args.check:
        failures = [
            path.relative_to(ROOT).as_posix()
            for path, text in generated.items()
            if not path.is_file() or path.read_text(encoding="utf-8") != text
        ]
        actual_locale_files = set(OUT_DIR.glob("fluent_catalog_*.vit"))
        stale_locale_files = actual_locale_files - set(generated)
        failures.extend(path.relative_to(ROOT).as_posix() for path in sorted(stale_locale_files))
        if failures:
            print("[frontend-fluent][error] stale generated catalog files:", file=sys.stderr)
            for failure in failures:
                print(f"  - {failure}", file=sys.stderr)
            return 1
        print(f"[frontend-fluent] check ok files={len(generated)}")
        return 0
    for path, text in generated.items():
        path.write_text(text, encoding="utf-8")
    print(f"[frontend-fluent] wrote files={len(generated)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
