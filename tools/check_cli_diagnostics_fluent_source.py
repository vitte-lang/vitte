#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "src/vitte/compiler/diagnostics/report.vit"
DRIVER = ROOT / "src/vitte/compiler/driver/compiler.vit"
CATALOG = ROOT / "src/vitte/compiler/infrastructure/diagnostics/fluent_catalog.vit"
EN = ROOT / "locales/en/diagnostics.ftl"
FR = ROOT / "locales/fr/diagnostics.ftl"

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
    report = REPORT.read_text(encoding="utf-8")
    driver = DRIVER.read_text(encoding="utf-8")
    catalog = CATALOG.read_text(encoding="utf-8")
    en = parse_ftl(EN)
    fr = parse_ftl(FR)

    if en.get(CODE) != EN_MESSAGE:
        errors.append(f"{EN.relative_to(ROOT)} must define {CODE} = {EN_MESSAGE!r}")
    if fr.get(CODE) != FR_MESSAGE:
        errors.append(f"{FR.relative_to(ROOT)} must define {CODE} = {FR_MESSAGE!r}")

    required_report_terms = (
        "use vitte/compiler/infrastructure/diagnostics/fluent_catalog",
        "proc render_text_lang(diagnostic0: TextDiagnostic, lang: string)",
        "fluent_catalog_normalize_locale(lang)",
        "fluent_catalog_lookup(locale, diagnostic0.code)",
        "if localized != \"\" { localized } else { diagnostic0.message }",
    )
    for term in required_report_terms:
        if term not in report:
            errors.append(f"{REPORT.relative_to(ROOT)} missing {term!r}")

    if "const FLUENT_DEFAULT_LOCALE: string = \"en\";" not in catalog:
        errors.append("generated Fluent catalog must default to en")
    if "if locale == \"\" {\n        give FLUENT_DEFAULT_LOCALE;" not in catalog:
        errors.append("generated Fluent catalog must fallback empty locale to en")
    if "if locale == \"fr\" or locale == \"fr-FR\"" not in catalog:
        errors.append("generated Fluent catalog must normalize fr aliases")

    signature = "proc print_result_diagnostics(result0: CompilerResult, lang: string)"
    if signature not in driver:
        errors.append(f"{DRIVER.relative_to(ROOT)} must make diagnostic printing locale-aware")
    if "report.render_text_lang(result0.diagnostics[i], lang)" not in driver:
        errors.append("driver diagnostic printing must use report.render_text_lang")

    call_sites = re.findall(r"print_result_diagnostics\(([^)]*)\)", driver)
    non_signature_calls = [site for site in call_sites if "CompilerResult" not in site]
    bad_calls = [site for site in non_signature_calls if "request.lang" not in site]
    if bad_calls:
        errors.append("all driver diagnostic print call sites must pass request.lang")
        errors.extend(f"bad call: print_result_diagnostics({site})" for site in bad_calls)
    if len(non_signature_calls) < 4:
        errors.append("expected check/build/run/test diagnostic print call sites")

    if errors:
        return fail(errors)

    print("[cli-diagnostics-fluent-source] status=ok default_locale=en localized_runtime_path=source")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
