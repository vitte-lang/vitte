#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "src" / "vitte" / "compiler" / "infrastructure" / "diagnostics" / "fluent_catalog.vit"
LOCALES = {
    "en": ROOT / "locales" / "en" / "diagnostics.ftl",
    "fr": ROOT / "locales" / "fr" / "diagnostics.ftl",
}
PREFIXES = ("LEX_", "P")


def parse_ftl(path: Path) -> dict[str, str]:
    data: dict[str, str] = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        data[key.strip()] = value.strip()
    return data


def is_frontend_code(code: str) -> bool:
    return any(code.startswith(prefix) for prefix in PREFIXES)


def vitte_escape(text: str) -> str:
    return text.replace("\\", "\\\\").replace('"', '\\"')


def render() -> str:
    locale_maps = {locale: parse_ftl(path) for locale, path in LOCALES.items()}
    codes = sorted({code for data in locale_maps.values() for code in data if is_frontend_code(code)})
    lines = [
        "space vitte/compiler/infrastructure/diagnostics/fluent_catalog",
        "",
        "# Generated from locales/*/diagnostics.ftl by tools/generate_frontend_fluent_bridge.py",
        "proc fluent_catalog_lookup(locale: string, code: string) -> string {",
    ]
    for locale in ("en", "fr"):
        lines.append(f'  if locale == "{locale}" {{')
        for code in codes:
            value = locale_maps[locale].get(code, "")
            lines.append(f'    if code == "{code}" {{')
            lines.append(f'      give "{vitte_escape(value)}";')
            lines.append("    }")
        lines.append("  }")
    lines.append('  give "";')
    lines.append("}")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    OUT.write_text(render(), encoding="utf-8")
    print(f"[frontend-fluent] wrote {OUT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
