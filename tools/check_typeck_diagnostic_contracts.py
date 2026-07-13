#!/usr/bin/env python3
from __future__ import annotations

import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CONTRACTS = ROOT / "schemas" / "diagnostics" / "typeck_contracts.json"
CODES = ROOT / "schemas" / "diagnostics" / "codes.json"
ERRORS = ROOT / "src" / "vitte" / "compiler" / "analysis" / "typeck" / "errors.vit"
TESTS = ROOT / "src" / "vitte" / "compiler" / "tests" / "typeck_tests.vit"
CATALOG = ROOT / "src" / "vitte" / "compiler" / "diagnostics" / "catalog.vit"
LOCALES = ROOT / "locales"
REPORT = ROOT / "target" / "reports" / "typeck_diagnostic_contracts.json"
EXPECTED_IDS = [f"DIAG-{number:04d}" for number in range(62, 69)]


def load_json(path: Path) -> object:
    with path.open(encoding="utf-8") as stream:
        return json.load(stream)


def fail(message: str) -> int:
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(
        json.dumps(
            {
                "schema": "vitte.typeck-diagnostic-contract-check",
                "schema_version": "1.0.0",
                "status": "fail",
                "message": message,
            },
            indent=2,
            sort_keys=True,
        )
        + "\n",
        encoding="utf-8",
    )
    print(f"[typeck-diagnostics][error] {message}", file=sys.stderr)
    return 1


def ftl_has_code(path: Path, code: str) -> bool:
    if not path.is_file():
        return False
    return re.search(rf"^{re.escape(code)}\s*=", path.read_text(encoding="utf-8"), re.MULTILINE) is not None


def main() -> int:
    payload = load_json(CONTRACTS)
    if not isinstance(payload, dict) or payload.get("schema") != "vitte.typeck-diagnostic-contracts":
        return fail(f"{CONTRACTS.relative_to(ROOT)} has an invalid schema identity")
    contracts = payload.get("contracts")
    if not isinstance(contracts, list):
        return fail(f"{CONTRACTS.relative_to(ROOT)} must contain a contracts array")

    ids = [entry.get("diag_id") for entry in contracts if isinstance(entry, dict)]
    codes = [entry.get("code") for entry in contracts if isinstance(entry, dict)]
    helpers = [entry.get("helper") for entry in contracts if isinstance(entry, dict)]
    if ids != EXPECTED_IDS:
        return fail(f"typeck DIAG ids must be exactly {', '.join(EXPECTED_IDS)}")
    if len(set(codes)) != len(codes):
        return fail("typeck diagnostic contract codes must be unique")
    if len(set(helpers)) != len(helpers):
        return fail("typeck diagnostic contract helpers must be unique")

    registry = load_json(CODES)
    if not isinstance(registry, dict) or not isinstance(registry.get("codes"), list):
        return fail(f"{CODES.relative_to(ROOT)} has an invalid code registry")
    public_codes = {entry.get("code") for entry in registry["codes"] if isinstance(entry, dict)}
    errors_text = ERRORS.read_text(encoding="utf-8")
    tests_text = TESTS.read_text(encoding="utf-8")
    catalog_text = CATALOG.read_text(encoding="utf-8")
    locale_files = sorted(LOCALES.glob("*/diagnostics.ftl"))
    if not locale_files:
        return fail("no diagnostics locales found")

    failures: list[str] = []
    if "proc enforce_typeck_cause_chains(" not in errors_text:
        failures.append("typeck result cause-chain enforcement helper is missing")
    if "enforce_typeck_cause_chains(diagnostics)" not in errors_text:
        failures.append("typeck result constructor does not enforce cause-chain contracts")
    if "test_typeck_result_enforces_diagnostic_cause_chains" not in tests_text:
        failures.append("typeck cause-chain enforcement regression test is missing")
    results: list[dict[str, object]] = []
    for entry in contracts:
        if not isinstance(entry, dict):
            failures.append("contract entry is not an object")
            continue
        diag_id = entry.get("diag_id")
        code = entry.get("code")
        helper = entry.get("helper")
        phase = entry.get("phase")
        production = entry.get("production")
        cause_chain = entry.get("cause_chain_required")
        status = "pass"
        reasons: list[str] = []
        if phase != "typeck":
            reasons.append("phase must be typeck")
        if production is not True:
            reasons.append("contract must be production=true")
        if cause_chain is not True:
            reasons.append("cause_chain_required must be true")
        if code not in public_codes:
            reasons.append(f"{code} missing from schemas/diagnostics/codes.json")
        if not isinstance(helper, str) or f"proc {helper}(" not in errors_text:
            reasons.append(f"{helper} helper missing from typeck/errors.vit")
        if isinstance(helper, str) and f"share {helper}" not in errors_text:
            reasons.append(f"{helper} is not shared from typeck/errors.vit")
        if isinstance(code, str) and code not in catalog_text:
            reasons.append(f"{code} missing from compiler diagnostics catalog")
        missing_locales = [
            str(path.parent.name)
            for path in locale_files
            if not isinstance(code, str) or not ftl_has_code(path, code)
        ]
        if missing_locales:
            reasons.append(f"{code} missing from locales: {', '.join(missing_locales)}")
        if reasons:
            status = "fail"
            failures.extend(f"{diag_id}: {reason}" for reason in reasons)
        results.append(
            {
                "diag_id": diag_id,
                "code": code,
                "helper": helper,
                "status": status,
                "reasons": reasons,
            }
        )

    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(
        json.dumps(
            {
                "schema": "vitte.typeck-diagnostic-contract-check",
                "schema_version": "1.0.0",
                "status": "fail" if failures else "pass",
                "results": results,
                "failures": failures,
            },
            indent=2,
            sort_keys=True,
        )
        + "\n",
        encoding="utf-8",
    )
    print(f"[typeck-diagnostics] status={'fail' if failures else 'pass'} report={REPORT.relative_to(ROOT)}")
    for failure in failures:
        print(f"[typeck-diagnostics][error] {failure}", file=sys.stderr)
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
