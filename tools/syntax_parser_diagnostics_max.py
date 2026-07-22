#!/usr/bin/env python3
"""Maximal syntax, parser, and frontend diagnostic release gate.

This gate sits above lexer-parser-coverage-100. It binds the complete public
lexer/parser diagnostic catalog to real snapshots, corpus checks, recovery
checks, schema checks, style checks, and deterministic fuzz smoke.
"""

from __future__ import annotations

import hashlib
import json
import subprocess
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "target/frontend/syntax_parser_diagnostics_max"
REPORT_DIR = ROOT / "target/reports"
VALID_FIXTURE = ROOT / "tests/frontend/syntax_parser_diagnostics_max/valid_surface.vit"
INVALID_FIXTURE = ROOT / "tests/frontend/syntax_parser_diagnostics_max/invalid_surface.vit"
VALID_CORPUS = (
    ROOT / "tests/frontend/coverage_100/valid_surface.vit",
    VALID_FIXTURE,
    ROOT / "tests/diagnostics/sources/accents.vit",
    ROOT / "tests/diagnostics/sources/ascii.vit",
    ROOT / "tests/diagnostics/sources/crlf.vit",
    ROOT / "tests/diagnostics/sources/empty.vit",
    ROOT / "tests/diagnostics/sources/lf.vit",
    ROOT / "tests/diagnostics/sources/long-line.vit",
    ROOT / "tests/diagnostics/sources/tabs.vit",
    ROOT / "tests/diagnostics/sources/types.vit",
    ROOT / "tests/diagnostics/sources/unicode.vit",
)
CATALOG = ROOT / "schemas/diagnostics/codes.json"


COMMANDS: tuple[tuple[str, ...], ...] = (
    ("python3", "tools/lexer_parser_coverage_100.py"),
    ("python3", "tools/check_lexer_diagnostics.py"),
    ("python3", "tools/check_parser_diagnostic_snapshots.py"),
    ("python3", "tools/check_diagnostic_schema.py"),
    ("python3", "tools/check_diagnostic_catalog.py"),
    ("python3", "tools/check_suggestion_quality.py"),
    ("python3", "tools/parser_lexer_fuzz_smoke.py", "--cases", "120", "--seed", "424242"),
)


MIN_VALID_METRICS = {
    "toplevel_count": 30,
    "stmt_count": 50,
    "expr_count": 90,
    "type_count": 60,
    "pattern_count": 30,
    "token_count": 750,
}
VAGUE_FRONTEND_TERMS = (
    "unknown error",
    "unexpected failure",
    "something went wrong",
    "fix the error",
    "try again",
)


def run(command: tuple[str, ...], expect_success: bool = True) -> subprocess.CompletedProcess[str]:
    proc = subprocess.run(command, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=120)
    if expect_success and proc.returncode != 0:
        raise SystemExit(
            f"[syntax-parser-max][error] command failed: {' '.join(command)}\n{proc.stdout}{proc.stderr}"
        )
    if not expect_success and proc.returncode == 0:
        raise SystemExit(f"[syntax-parser-max][error] command unexpectedly passed: {' '.join(command)}")
    return proc


def load_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def digest(value: object) -> str:
    payload = json.dumps(value, ensure_ascii=True, sort_keys=True, separators=(",", ":")).encode("utf-8")
    return hashlib.sha256(payload).hexdigest()


def public_frontend_codes() -> list[dict[str, Any]]:
    payload = load_json(CATALOG)
    codes = payload.get("codes")
    if not isinstance(codes, list):
        raise SystemExit("[syntax-parser-max][error] diagnostic code catalog is malformed")
    selected = [entry for entry in codes if isinstance(entry, dict) and entry.get("phase") in {"lexer", "parser"}]
    selected.sort(key=lambda entry: (str(entry["phase"]), str(entry["code"])))
    lexer_count = sum(1 for entry in selected if entry.get("phase") == "lexer")
    parser_count = sum(1 for entry in selected if entry.get("phase") == "parser")
    if lexer_count != 21 or parser_count != 105:
        raise SystemExit(
            f"[syntax-parser-max][error] expected 21 lexer and 105 parser codes, got {lexer_count}/{parser_count}"
        )
    return selected


def snapshot_path(entry: dict[str, Any]) -> Path:
    phase = str(entry["phase"])
    code = str(entry["code"])
    return ROOT / "tests/diagnostics/catalog/snapshots" / phase / f"{code}.snap"


def validate_catalog_rows(codes: list[dict[str, Any]]) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    for entry in codes:
        code = str(entry.get("code", ""))
        phase = str(entry.get("phase", ""))
        documentation = entry.get("documentation")
        tests = entry.get("tests")
        path = snapshot_path(entry)
        if entry.get("stable") is not True:
            raise SystemExit(f"[syntax-parser-max][error] {code} is not stable")
        if entry.get("deprecated") not in {True, False}:
            raise SystemExit(f"[syntax-parser-max][error] {code} lacks deprecated=true/false")
        if not isinstance(documentation, dict):
            raise SystemExit(f"[syntax-parser-max][error] {code} lacks documentation")
        missing_doc = {"title", "summary", "cause", "action", "example", "url"} - set(documentation)
        if missing_doc:
            raise SystemExit(f"[syntax-parser-max][error] {code} documentation misses {sorted(missing_doc)}")
        for field in ("title", "summary", "cause", "action"):
            text = str(documentation[field]).lower()
            vague = [term for term in VAGUE_FRONTEND_TERMS if term in text]
            if vague:
                raise SystemExit(f"[syntax-parser-max][error] {code} documentation.{field} is vague: {vague}")
        if not isinstance(tests, list) or not tests:
            raise SystemExit(f"[syntax-parser-max][error] {code} has no mapped tests")
        if not path.is_file() or path.stat().st_size == 0:
            raise SystemExit(f"[syntax-parser-max][error] missing catalog snapshot for {code}: {path.relative_to(ROOT)}")
        snapshot_text = path.read_text(encoding="utf-8", errors="replace")
        for needle in (code, phase, str(entry.get("message_key"))):
            if needle not in snapshot_text:
                raise SystemExit(f"[syntax-parser-max][error] {path.relative_to(ROOT)} misses {needle!r}")
        rows.append(
            {
                "code": code,
                "phase": phase,
                "title": documentation["title"],
                "message_key": entry.get("message_key"),
                "snapshot": path.relative_to(ROOT).as_posix(),
                "tests": tests,
                "stable": True,
                "deprecated": entry.get("deprecated"),
                "documentation_url": documentation["url"],
                "fingerprint": digest(
                    {
                        "code": code,
                        "phase": phase,
                        "title": documentation["title"],
                        "snapshot": snapshot_text,
                        "tests": tests,
                    }
                )[:16],
            }
        )
    return rows


def frontend_syntax_json(paths: tuple[Path, ...]) -> list[dict[str, Any]]:
    command = ("python3", "tools/frontend_syntax_check.py", "--json", *(path.as_posix() for path in paths))
    proc = run(command)
    return json.loads(proc.stdout)


def validate_frontend_corpus() -> dict[str, Any]:
    valid_results = frontend_syntax_json(VALID_CORPUS)
    failures = [item for item in valid_results if not item.get("valid")]
    if failures:
        raise SystemExit(f"[syntax-parser-max][error] valid corpus failed: {failures}")
    max_result = next(item for item in valid_results if item.get("path") == str(VALID_FIXTURE))
    for metric, minimum in MIN_VALID_METRICS.items():
        value = int(max_result.get(metric, 0))
        if value < minimum:
            raise SystemExit(f"[syntax-parser-max][error] {metric}={value} below minimum {minimum}")

    bad = run(("python3", "tools/frontend_syntax_check.py", "--json", INVALID_FIXTURE.as_posix()), expect_success=False)
    invalid_results = json.loads(bad.stdout)
    diagnostics = invalid_results[0].get("diagnostics", []) if invalid_results else []
    if not isinstance(diagnostics, list) or len(diagnostics) < 4:
        raise SystemExit("[syntax-parser-max][error] invalid fixture must produce at least four parser diagnostics")
    return {
        "valid_files": len(valid_results),
        "valid_max_metrics": {key: max_result[key] for key in MIN_VALID_METRICS},
        "invalid_diagnostics": diagnostics,
    }


def render_manifest(rows: list[dict[str, Any]], corpus: dict[str, Any]) -> dict[str, Any]:
    requirements = []
    for index, row in enumerate(rows, start=1):
        item = dict(row)
        item["requirement"] = f"SPD{index:03d}"
        item["status"] = "covered"
        item["explanation"] = f"{row['code']} is stable, documented, tested, and snapshot-backed for the frontend."
        item["suggestion"] = "Keep the fixture, catalog entry, and snapshot synchronized before changing parser behavior."
        requirements.append(item)
    return {
        "schema": "vitte.syntax_parser_diagnostics.max",
        "schema_version": "1.0.0",
        "coverage_percent": 100,
        "requirements_total": len(requirements),
        "requirements_covered": len(requirements),
        "lexer_codes": sum(1 for row in rows if row["phase"] == "lexer"),
        "parser_codes": sum(1 for row in rows if row["phase"] == "parser"),
        "commands": [" ".join(command) for command in COMMANDS],
        "corpus": corpus,
        "matrix_fingerprint": digest(requirements),
        "requirements": requirements,
    }


def write_outputs(manifest: dict[str, Any]) -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    (OUT_DIR / "snapshots").mkdir(parents=True, exist_ok=True)
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    manifest_text = json.dumps(manifest, ensure_ascii=True, indent=2, sort_keys=True) + "\n"
    requirements = manifest["requirements"]
    text_snapshot = "\n".join(
        f"{row['requirement']} covered {row['phase']} {row['code']} {row['fingerprint']} {row['title']}"
        for row in requirements
    ) + "\n"
    lsp = {
        "jsonrpc": "2.0",
        "method": "textDocument/publishDiagnostics",
        "params": {
            "uri": "file:///" + INVALID_FIXTURE.relative_to(ROOT).as_posix(),
            "version": 1,
            "diagnostics": [
                {
                    "source": "vitte",
                    "severity": 1,
                    "code": row["code"],
                    "message": row["explanation"],
                    "data": {"requirement": row["requirement"], "phase": row["phase"], "fix_why": row["suggestion"]},
                }
                for row in requirements
            ],
        },
    }
    lsp_text = json.dumps(lsp, ensure_ascii=True, indent=2, sort_keys=True) + "\n"
    md_lines = [
        "# Syntax Parser Diagnostics Max",
        "",
        f"Coverage: {manifest['requirements_covered']}/{manifest['requirements_total']} ({manifest['coverage_percent']}%)",
        f"Lexer codes: {manifest['lexer_codes']}",
        f"Parser codes: {manifest['parser_codes']}",
        f"Fingerprint: {manifest['matrix_fingerprint']}",
        "",
        "| ID | Phase | Code | Status | Title |",
        "| --- | --- | --- | --- | --- |",
    ]
    for row in requirements:
        md_lines.append(f"| {row['requirement']} | {row['phase']} | {row['code']} | {row['status']} | {row['title']} |")
    md = "\n".join(md_lines) + "\n"
    (OUT_DIR / "manifest.json").write_text(manifest_text, encoding="utf-8")
    (OUT_DIR / "coverage.md").write_text(md, encoding="utf-8")
    (OUT_DIR / "snapshots/text.snapshot").write_text(text_snapshot, encoding="utf-8")
    (OUT_DIR / "snapshots/json.snapshot").write_text(manifest_text, encoding="utf-8")
    (OUT_DIR / "snapshots/lsp.snapshot").write_text(lsp_text, encoding="utf-8")
    (REPORT_DIR / "syntax_parser_diagnostics_max.json").write_text(manifest_text, encoding="utf-8")
    (REPORT_DIR / "syntax_parser_diagnostics_max.md").write_text(md, encoding="utf-8")


def validate_outputs(manifest: dict[str, Any]) -> None:
    if manifest["requirements_total"] != 126 or manifest["coverage_percent"] != 100:
        raise SystemExit("[syntax-parser-max][error] manifest must cover exactly 126 frontend public codes")
    required = (
        OUT_DIR / "manifest.json",
        OUT_DIR / "coverage.md",
        OUT_DIR / "snapshots/text.snapshot",
        OUT_DIR / "snapshots/json.snapshot",
        OUT_DIR / "snapshots/lsp.snapshot",
        REPORT_DIR / "syntax_parser_diagnostics_max.json",
        REPORT_DIR / "syntax_parser_diagnostics_max.md",
    )
    for path in required:
        if not path.is_file() or path.stat().st_size == 0:
            raise SystemExit(f"[syntax-parser-max][error] missing generated artifact: {path.relative_to(ROOT)}")


def main() -> int:
    for command in COMMANDS:
        run(command)
    codes = public_frontend_codes()
    rows = validate_catalog_rows(codes)
    corpus = validate_frontend_corpus()
    manifest = render_manifest(rows, corpus)
    write_outputs(manifest)
    validate_outputs(manifest)
    print(
        "[syntax-parser-max] OK "
        f"requirements={manifest['requirements_total']} "
        f"lexer={manifest['lexer_codes']} parser={manifest['parser_codes']} "
        f"fingerprint={manifest['matrix_fingerprint']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
