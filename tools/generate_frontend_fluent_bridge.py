#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "src" / "vitte" / "compiler" / "infrastructure" / "diagnostics" / "fluent_catalog.vit"
CORE_CODES = ROOT / "tests" / "diag_snapshots" / "core_diagnostic_codes.txt"
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
        "# Generated from locales/*/diagnostics.ftl by tools/generate_frontend_fluent_bridge.py",
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
        lines.append(f"proc fluent_catalog_lookup_{fn_suffix}(code: string) -> string {{")
        for code in codes:
            value = locale_maps[locale].get(code, "")
            lines.append(f'    if code == "{code}" {{')
            lines.append(f'        give "{vitte_escape(value)}";')
            lines.append("    }")
            lines.append("")
        lines.append('    give "";')
        lines.append("}")
        lines.append("")
    lines.extend([
        "proc fluent_catalog_lookup(locale: string, code: string) -> string {",
        "    let normalized: string = fluent_catalog_normalize_locale(locale);",
        "",
    ])
    for locale in supported_locale_codes():
        lines.extend([
            f'    if normalized == "{locale}" {{',
            f"        give fluent_catalog_lookup_{symbol_for_locale(locale)}(code);",
            "    }",
            "",
        ])
    lines.extend([
        '    give "";',
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


def main() -> int:
    OUT.write_text(render(), encoding="utf-8")
    print(f"[frontend-fluent] wrote {OUT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
