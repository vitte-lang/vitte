#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from pathlib import Path
from typing import Any

import compiler_real_diagnostics_gate as real


ROOT = Path(__file__).resolve().parents[1]
CORPUS = ROOT / "tests" / "compiler_real_diagnostics" / "corpus.vitte.json"
INVALID_ROOT = ROOT / "tests" / "compiler_real_diagnostics" / "invalid"
REPORT_DIR = ROOT / "target" / "reports" / "invalid_fixtures_contract"
REPORT_JSON = REPORT_DIR / "coverage.json"
REPORT_MD = REPORT_DIR / "coverage.md"


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def invalid_fixture_entries() -> set[str]:
    return {
        rel(path)
        for path in sorted(INVALID_ROOT.rglob("*.vit"))
        if path.is_file()
    }


def validate_fixture_coverage(cases: list[dict[str, Any]]) -> list[str]:
    failures: list[str] = []
    entries = {str(case.get("entry", "")) for case in cases}
    invalid_entries = invalid_fixture_entries()
    missing = sorted(invalid_entries - entries)
    if missing:
        failures.append(f"invalid fixtures missing from strict manifest: {missing}")
    extra_invalid = sorted(entry for entry in entries if entry.startswith("tests/compiler_real_diagnostics/invalid/") and entry not in invalid_entries)
    if extra_invalid:
        failures.append(f"manifest entries point to missing invalid fixtures: {extra_invalid}")
    return failures


def render_markdown(report: dict[str, Any]) -> str:
    lines = [
        "# Invalid Fixtures Contract",
        "",
        f"- status: `{report['status']}`",
        f"- locale: `{report['locale']}`",
        f"- passed: `{report['passed']}/{report['total']}`",
        f"- pending: `{report['pending_count']}`",
        "",
        "| case | phase | command | status | diagnostics |",
        "| --- | --- | --- | --- | --- |",
    ]
    for case in report["cases"]:
        lines.append(
            f"| `{case['id']}` | `{case['phase']}` | `{case['command_name']}` | `{case['status']}` | `{', '.join(case['expected_codes'])}` |"
        )
    if report["failures"]:
        lines.extend(["", "## Failures"])
        lines.extend(f"- {failure}" for failure in report["failures"][:120])
    return "\n".join(lines) + "\n"


def main() -> int:
    if not real.BIN.is_file():
        print(f"[invalid-fixtures-contract][error] missing compiler: {rel(real.BIN)}", file=sys.stderr)
        return 1
    payload = real.load_json(CORPUS)
    locale = str(payload.get("locale", "fr"))
    messages = real.parse_ftl(ROOT / "locales" / locale / "diagnostics.ftl")
    raw_cases = payload.get("cases", [])
    pending = payload.get("pending_cases", [])
    failures: list[str] = []
    if not isinstance(raw_cases, list) or not raw_cases:
        failures.append(f"{rel(CORPUS)} must contain non-empty cases")
        cases: list[dict[str, Any]] = []
    else:
        cases = [case for case in raw_cases if isinstance(case, dict)]
        if len(cases) != len(raw_cases):
            failures.append("all cases must be objects")

    if pending:
        failures.append("strict invalid fixture gate forbids pending_cases")
    if not isinstance(pending, list):
        failures.append("pending_cases must be a list")

    for case in cases:
        failures.extend(real.validate_manifest_case(case, "cases"))
        if case.get("expected_locale") != locale:
            failures.append(f"case {case.get('id', '<missing-id>')} expected_locale must match corpus locale {locale!r}")
        if case.get("command") not in {"check", "build"}:
            failures.append(f"case {case.get('id', '<missing-id>')} must run through bin/vitte check/build")

    failures.extend(validate_fixture_coverage(cases))
    results = [real.validate_case(case, locale, messages) for case in cases]
    failures.extend(
        f"{result['id']}: {failure}"
        for result in results
        for failure in result["failures"]
    )

    passed = sum(1 for result in results if result["status"] == "pass")
    report = {
        "schema": "vitte.invalid_fixtures_contract.coverage.v1",
        "status": "pass" if not failures else "fail",
        "corpus": rel(CORPUS),
        "locale": locale,
        "pending_count": len(pending) if isinstance(pending, list) else -1,
        "invalid_fixture_count": len(invalid_fixture_entries()),
        "passed": passed,
        "total": len(results),
        "cases": results,
        "failures": failures,
    }
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    REPORT_JSON.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    REPORT_MD.write_text(render_markdown(report), encoding="utf-8")

    if failures:
        for failure in failures[:80]:
            print(f"[invalid-fixtures-contract][error] {failure}", file=sys.stderr)
        print(f"[invalid-fixtures-contract] report={rel(REPORT_JSON)}", file=sys.stderr)
        return 1
    print(f"[invalid-fixtures-contract] OK passed={passed}/{len(results)} report={rel(REPORT_JSON)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
