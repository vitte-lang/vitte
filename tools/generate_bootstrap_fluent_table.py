#!/usr/bin/env python3
from __future__ import annotations

import argparse
import difflib
import sys
from pathlib import Path

from diagnostics_locales import SUPPORTED_DIAGNOSTIC_LOCALES


ROOT = Path(__file__).resolve().parents[1]
MODULE = ROOT / "bootstrap/src/backend/c17/module.c"

BOOTSTRAP_CODES = (
    "E_CLI_MISSING_ARG",
    "E_CLI_UNKNOWN_COMMAND",
    "E_CLI_UNKNOWN_OPTION",
    "E_CLI_INVALID_ARGUMENT",
    "E_IO_FILE_NOT_FOUND",
    "E_IO_FILE_UNREADABLE",
    "E_IO_OUTPUT_UNWRITABLE",
    "E_IO_DIRECTORY_NOT_FOUND",
    "E_IO_OVERWRITE_FORBIDDEN",
    "LINK_E_OUTPUT_NOT_MATERIALIZED",
    "BACKEND_E_MISSING_C_COMPILER",
    "BACKEND_E_UNSUPPORTED_TARGET",
    "LEX_E_INVALID_CHAR",
    "PARSE_E_UNCLOSED_BLOCK",
    "MOD_E_MODULE_NOT_FOUND",
    "SEMA_E_UNKNOWN_IDENTIFIER",
    "TYPECK_E_ASSIGN_MISMATCH",
    "TYPECK_E_RETURN_MISMATCH",
    "BORROWCK_E_USE_AFTER_MOVE",
    "MIR_E_VERIFICATION_FAILED",
    "IR_E_VERIFY_FAILED",
)


def parse_ftl(path: Path) -> dict[str, str]:
    entries: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if " =" not in line:
            continue
        key, value = line.split("=", 1)
        entries[key.strip()] = value.strip()
    return entries


def c_escape(text: str) -> str:
    return text.replace("\\", "\\\\").replace('"', '\\"')


def module_line(source_line: str) -> str:
    return f'            "{c_escape(source_line)}",'


def generated_source_lines() -> list[str]:
    catalogs = {
        locale.code: parse_ftl(ROOT / "locales" / locale.code / "diagnostics.ftl")
        for locale in SUPPORTED_DIAGNOSTIC_LOCALES
    }
    missing = [
        f"{locale.code}:{code}"
        for locale in SUPPORTED_DIAGNOSTIC_LOCALES
        for code in BOOTSTRAP_CODES
        if code not in catalogs[locale.code]
    ]
    if missing:
        preview = ", ".join(missing[:12])
        raise RuntimeError(f"missing bootstrap Fluent messages: {preview}")

    lines: list[str] = []
    lines.append("static const char *vitte_locale_normalize(const char *locale) {")
    lines.append("    if (locale == NULL || locale[0] == '\\0') return \"en\";")
    for locale in SUPPORTED_DIAGNOSTIC_LOCALES:
        prefix = locale.code.split("-", 1)[0]
        lines.append(
            f"    if (strncmp(locale, \"{prefix}\", {len(prefix)}u) == 0) return \"{locale.code}\";"
        )
        for alias in locale.aliases:
            lines.append(f"    if (strcmp(locale, \"{alias}\") == 0) return \"{locale.code}\";")
    lines.append("    return \"en\";")
    lines.append("}")
    lines.append("static const char *vitte_args_locale(int argc, char **argv) {")
    lines.append("    const char *locale = getenv(\"VITTE_LANG\");")
    lines.append("    int index;")
    lines.append("    for (index = 1; index < argc; index++) {")
    lines.append("        if (strcmp(argv[index], \"--lang\") == 0) {")
    lines.append(
        "            if (index + 1 < argc && argv[index + 1] != NULL && argv[index + 1][0] != '\\0' && argv[index + 1][0] != '-') {"
    )
    lines.append("                locale = argv[index + 1];")
    lines.append("                index++;")
    lines.append("            }")
    lines.append("        } else if (strncmp(argv[index], \"--lang=\", 7u) == 0) {")
    lines.append("            locale = argv[index] + 7;")
    lines.append("        }")
    lines.append("    }")
    lines.append("    return vitte_locale_normalize(locale);")
    lines.append("}")
    lines.append("static const char *vitte_diag_message_for(const char *locale, const char *code) {")
    lines.append("    if (code == NULL) {")
    lines.append("        return \"diagnostic\";")
    lines.append("    }")
    lines.append("    locale = vitte_locale_normalize(locale);")
    for locale in SUPPORTED_DIAGNOSTIC_LOCALES:
        if locale.code == "en":
            continue
        head = "if" if locale.code != "fr" else "if"
        lines.append(f"    {head} (strcmp(locale, \"{locale.code}\") == 0) {{")
        for code in BOOTSTRAP_CODES:
            lines.append(
                f"        if (strcmp(code, \"{code}\") == 0) return \"{catalogs[locale.code][code]}\";"
            )
        lines.append("    }")
    for code in BOOTSTRAP_CODES:
        lines.append(f"    if (strcmp(code, \"{code}\") == 0) return \"{catalogs['en'][code]}\";")
    lines.append("    return \"diagnostic bootstrap\";")
    lines.append("}")
    return lines


def generated_module_lines() -> list[str]:
    return [module_line(line) for line in generated_source_lines()]


def rewrite_module(text: str) -> str:
    lines = text.splitlines()
    start = next(
        index for index, line in enumerate(lines)
        if '"static const char *vitte_locale_normalize(const char *locale) {"' in line
    )
    end = next(
        index for index, line in enumerate(lines[start:], start)
        if '"static void vitte_diag_emit_fields_json(const char *locale, const char *code, const char *span) {"' in line
    )
    return "\n".join(lines[:start] + generated_module_lines() + lines[end:]) + "\n"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="sync bootstrap Fluent messages from locales/*.ftl")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args(argv)

    current = MODULE.read_text(encoding="utf-8")
    updated = rewrite_module(current)
    if args.check:
        if current != updated:
            diff = "\n".join(
                difflib.unified_diff(
                    current.splitlines(),
                    updated.splitlines(),
                    fromfile=str(MODULE.relative_to(ROOT)),
                    tofile=str(MODULE.relative_to(ROOT)),
                    lineterm="",
                )
            )
            print("[bootstrap-fluent-table][error] generated table is stale", file=sys.stderr)
            print(diff[:12000], file=sys.stderr)
            return 1
        print(f"[bootstrap-fluent-table] check ok {MODULE.relative_to(ROOT)}")
        return 0

    if current != updated:
        MODULE.write_text(updated, encoding="utf-8")
        print(f"[bootstrap-fluent-table] wrote {MODULE.relative_to(ROOT)}")
    else:
        print(f"[bootstrap-fluent-table] unchanged {MODULE.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
