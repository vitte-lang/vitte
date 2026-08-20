#!/usr/bin/env python3
"""Check text, JSON/LSP, and Fluent output surfaces against the driver source."""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DRIVER = ROOT / "src/vitte/compiler/driver/compiler.vit"
TEXT = ROOT / "src/vitte/compiler/diagnostics/render.vit"
JSON = ROOT / "src/vitte/compiler/diagnostics/json.vit"
LSP = ROOT / "src/vitte/compiler/diagnostics/lsp.vit"
FLUENT = ROOT / "src/vitte/compiler/infrastructure/diagnostics/fluent_catalog.vit"
REPORT_DIR = ROOT / "target/reports"
REPORT = REPORT_DIR / "diagnostic_output_stability.json"
MARKDOWN = REPORT_DIR / "diagnostic_output_stability.md"


def ftl_keys(path: Path) -> set[str]:
    return {
        line.split("=", 1)[0].strip()
        for line in path.read_text(encoding="utf-8").splitlines()
        if "=" in line and not line.lstrip().startswith("#")
    }


def main() -> int:
    failures: list[str] = []
    driver_text = DRIVER.read_text(encoding="utf-8")
    text = TEXT.read_text(encoding="utf-8")
    json_text = JSON.read_text(encoding="utf-8")
    lsp = LSP.read_text(encoding="utf-8")
    fluent = FLUENT.read_text(encoding="utf-8")
    driver_codes = sorted(set(re.findall(r'"((?:[A-Z][A-Z0-9]*_)+(?:E|W|I)_[A-Z0-9_]+)"', driver_text)))
    if not driver_codes:
        failures.append("driver has no structured diagnostic codes")

    required_text = (
        "proc render_terminal_report(",
        "render_header(",
        "render_labels(",
        "render_notes(",
        "render_helps(",
        "render_suggestions(",
    )
    required_json = (
        "proc json_diagnostic(",
        "proc json_report(",
        "proc json_diagnostic_output(",
        '"schema_version"',
        '"primary_span"',
        '"secondary_spans"',
    )
    required_lsp = (
        "proc lsp_diagnostic_json_value(",
        "proc lsp_publish_diagnostics_json_rpc(",
        "relatedInformation",
        '"jsonrpc"',
        '"textDocument/publishDiagnostics"',
    )
    for needle in required_text:
        if needle not in text:
            failures.append(f"text renderer missing `{needle}`")
    for needle in required_json:
        if needle not in json_text:
            failures.append(f"JSON renderer missing `{needle}`")
    for needle in required_lsp:
        if needle not in lsp:
            failures.append(f"LSP renderer missing `{needle}`")

    en = ftl_keys(ROOT / "locales/en/diagnostics.ftl")
    fr = ftl_keys(ROOT / "locales/fr/diagnostics.ftl")
    missing_en = sorted(code for code in driver_codes if code not in en)
    missing_fr = sorted(code for code in driver_codes if code not in fr)
    if missing_en:
        failures.append("driver codes missing from en Fluent: " + ", ".join(missing_en))
    if missing_fr:
        failures.append("driver codes missing from fr Fluent: " + ", ".join(missing_fr))
    for code in driver_codes:
        if f'if code == "{code}"' not in fluent:
            failures.append(f"generated Fluent catalog missing `{code}`")

    report = {
        "schema": "vitte.compiler.diagnostic-output-stability",
        "status": "error" if failures else "ok",
        "driver_code_count": len(driver_codes),
        "text_contracts": len(required_text),
        "json_contracts": len(required_json),
        "lsp_contracts": len(required_lsp),
        "failures": failures,
    }
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(__import__("json").dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    MARKDOWN.write_text(
        "# Diagnostic output stability\n\n"
        f"- driver codes: {report['driver_code_count']}\n"
        f"- text contracts: {report['text_contracts']}\n"
        f"- JSON contracts: {report['json_contracts']}\n"
        f"- LSP contracts: {report['lsp_contracts']}\n"
        f"- status: {report['status']}\n",
        encoding="utf-8",
    )
    if failures:
        for failure in failures:
            print(f"[diagnostic-output][error] {failure}", file=sys.stderr)
        return 1
    print(
        "[diagnostic-output] "
        f"driver_codes={report['driver_code_count']} text={report['text_contracts']} "
        f"json={report['json_contracts']} lsp={report['lsp_contracts']} status=ok"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
