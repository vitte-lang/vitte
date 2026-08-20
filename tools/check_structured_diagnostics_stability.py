#!/usr/bin/env python3
"""Check the canonical structured diagnostic model and serializers."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SCHEMA = ROOT / "schemas/diagnostics/v1.schema.json"
CODES = ROOT / "schemas/diagnostics/codes.json"
DIAGNOSTIC = ROOT / "src/vitte/compiler/diagnostics/diagnostic.vit"
JSON = ROOT / "src/vitte/compiler/diagnostics/json.vit"
LSP = ROOT / "src/vitte/compiler/diagnostics/lsp.vit"
SARIF = ROOT / "src/vitte/compiler/diagnostics/sarif.vit"
INFRA = ROOT / "src/vitte/compiler/infrastructure/diagnostics/diagnostic.vit"
REPORT_DIR = ROOT / "target/reports"
REPORT = REPORT_DIR / "structured_diagnostics_stability.json"
MARKDOWN = REPORT_DIR / "structured_diagnostics_stability.md"

REQUIRED_SCHEMA_FIELDS = {
    "schema",
    "schema_version",
    "code",
    "severity",
    "phase",
    "message_key",
    "message",
    "primary_span",
    "labels",
    "notes",
    "helps",
    "suggestions",
}

REQUIRED_SURFACES = {
    DIAGNOSTIC: (
        "form Diagnostic {",
        "form DiagnosticReport {",
        "form PrimarySpan {",
        "form SecondarySpan {",
        "schema_version: string",
        "primary_span: SourceSpan",
        "secondary_spans: [SourceSpan]",
        "internal_cause: string",
        "proc diagnostic_create(",
        "proc diagnostic_with_internal_cause(",
        "proc primary_span(",
        "proc secondary_span(",
        "proc diagnostic_is_root(",
    ),
    JSON: (
        "proc json_source_span(",
        "proc json_labels(",
        "proc json_suggestions(",
        "proc json_diagnostic(",
        "proc json_report(",
        "proc json_envelope(",
        '"schema_version"',
        '"primary_span"',
        '"secondary_spans"',
        '"internal_cause"',
    ),
    LSP: (
        "proc lsp_diagnostic_data_json(",
        "proc lsp_diagnostic_json_value(",
        "proc lsp_publish_diagnostics_json_rpc(",
        "relatedInformation",
    ),
    SARIF: (
        "proc sarif_result(",
        "proc sarif_report(",
        "level",
        "locations",
        "ruleId",
    ),
    INFRA: (
        "form InfrastructureDiagnostic {",
        "secondary_spans: [SourceSpan]",
        "primary_span: SourceSpan",
        "internal_cause: string",
        "proc diagnostic_create(",
        "proc diagnostic_with_internal_cause(",
    ),
}


def main() -> int:
    failures: list[str] = []
    contract_count = 0
    for path, needles in REQUIRED_SURFACES.items():
        text = path.read_text(encoding="utf-8") if path.is_file() else ""
        if not path.is_file():
            failures.append(f"missing structured diagnostic module: {path.relative_to(ROOT)}")
        for needle in needles:
            contract_count += 1
            if needle not in text:
                failures.append(f"{path.relative_to(ROOT)}: missing `{needle}`")

    if not SCHEMA.is_file():
        failures.append("missing diagnostics JSON schema")
    else:
        schema = json.loads(SCHEMA.read_text(encoding="utf-8"))
        required = set(schema.get("required", []))
        if required != REQUIRED_SCHEMA_FIELDS:
            failures.append("diagnostic schema required fields drifted")
        if schema.get("properties", {}).get("primary_span", {}).get("$ref") != "#/$defs/span":
            failures.append("diagnostic schema primary_span must use the canonical span definition")

    code_count = 0
    if not CODES.is_file():
        failures.append("missing diagnostic code registry")
    else:
        codes = json.loads(CODES.read_text(encoding="utf-8")).get("codes", [])
        code_count = len(codes) if isinstance(codes, list) else 0
        names = [entry.get("code") for entry in codes if isinstance(entry, dict)]
        if not names or len(names) != len(set(names)):
            failures.append("diagnostic code registry must contain unique non-empty codes")
        if any(not isinstance(name, str) or not re.fullmatch(r"(LEX|PAR|RES|SEM|TYP|BOR|MIR|IR|GEN|LNK|ICE)[0-9]{4}", name) for name in names):
            failures.append("diagnostic code registry contains a non-canonical public code")

    report = {
        "schema": "vitte.compiler.structured-diagnostics-stability",
        "status": "error" if failures else "ok",
        "contract_count": contract_count,
        "code_count": code_count,
        "failures": failures,
    }
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    MARKDOWN.write_text(
        "# Structured diagnostics stability\n\n"
        f"- contracts: {report['contract_count']}\n"
        f"- registered codes: {report['code_count']}\n"
        f"- status: {report['status']}\n",
        encoding="utf-8",
    )
    if failures:
        for failure in failures:
            print(f"[structured-diagnostics][error] {failure}", file=sys.stderr)
        return 1
    print(
        "[structured-diagnostics] "
        f"contracts={report['contract_count']} codes={report['code_count']} status=ok"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
