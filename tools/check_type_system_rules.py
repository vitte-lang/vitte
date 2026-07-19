#!/usr/bin/env python3
from __future__ import annotations

import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RULES = ROOT / "schemas" / "type_system" / "rules.json"
SPEC = ROOT / "docs" / "spec" / "type_system.md"
TESTS = ROOT / "src" / "vitte" / "compiler" / "tests" / "typeck_tests.vit"
CODES = ROOT / "schemas" / "diagnostics" / "codes.json"
REPORT = ROOT / "target" / "reports" / "type_system_rules.json"

REQUIRED_RULES = {
    "T-LITERAL", "T-NAME", "T-DECLARED-TYPE", "T-ASSIGN", "T-RETURN",
    "T-UNARY", "T-BINARY", "T-CONDITION", "T-BRANCH", "T-CALL",
    "T-GENERIC-INFER", "T-GENERIC-BOUND", "T-MEMBER", "T-METHOD",
    "T-CAST", "T-COERCE", "T-IMPL", "T-COHERENCE",
    "T-CONSTRAINT-ACYCLIC", "T-MATCH-EXHAUSTIVE",
}


def fail(message: str) -> int:
    print(f"[type-system-rules][error] {message}", file=sys.stderr)
    return 1


def main() -> int:
    payload = json.loads(RULES.read_text(encoding="utf-8"))
    if payload.get("schema") != "vitte.type-system-rules" or payload.get("schema_version") != "1.0.0":
        return fail("invalid rules schema identity or version")
    rules = payload.get("rules")
    judgments = payload.get("judgments")
    if not isinstance(rules, list) or not isinstance(judgments, list) or not judgments:
        return fail("rules and judgments must be non-empty arrays")

    ids = [rule.get("id") for rule in rules if isinstance(rule, dict)]
    failures: list[str] = []
    if len(ids) != len(set(ids)):
        failures.append("rule ids are not unique")
    if set(ids) != REQUIRED_RULES:
        failures.append(f"rule ids differ from required set: {sorted(set(ids) ^ REQUIRED_RULES)}")

    tests_text = TESTS.read_text(encoding="utf-8")
    spec_text = SPEC.read_text(encoding="utf-8")
    registry = json.loads(CODES.read_text(encoding="utf-8"))
    known_codes: set[object] = set()
    for entry in registry.get("codes", []):
        if not isinstance(entry, dict):
            continue
        known_codes.add(entry.get("code"))
        known_codes.add(entry.get("message_key"))
        aliases = entry.get("aliases")
        if isinstance(aliases, list):
            known_codes.update(aliases)
    results: list[dict[str, object]] = []

    for rule in rules:
        if not isinstance(rule, dict):
            failures.append("non-object rule entry")
            continue
        rule_id = rule.get("id")
        reasons: list[str] = []
        if rule.get("judgment") not in judgments:
            reasons.append("unknown judgment")
        if not isinstance(rule.get("premises"), list) or not rule["premises"]:
            reasons.append("premises must be non-empty")
        if not isinstance(rule.get("conclusion"), str) or not rule["conclusion"].strip():
            reasons.append("conclusion must be non-empty")
        if not isinstance(rule_id, str) or f"## {rule_id}" not in spec_text:
            reasons.append("rule is missing from the normative specification")

        implementation = rule.get("implementation")
        if not isinstance(implementation, dict):
            reasons.append("implementation mapping is missing")
        else:
            rel = implementation.get("file")
            symbol = implementation.get("symbol")
            path = ROOT / rel if isinstance(rel, str) else ROOT / "__missing__"
            if not path.is_file():
                reasons.append(f"implementation file is missing: {rel}")
            elif not isinstance(symbol, str) or re.search(rf"\bproc\s+{re.escape(symbol)}\s*\(", path.read_text(encoding="utf-8")) is None:
                reasons.append(f"implementation procedure is missing: {symbol}")

        tests = rule.get("tests")
        if not isinstance(tests, list) or not tests:
            reasons.append("at least one production test is required")
        else:
            for test in tests:
                if not isinstance(test, str) or f"proc {test}(" not in tests_text:
                    reasons.append(f"test procedure is missing: {test}")
                elif f"if not {test}()" not in tests_text:
                    reasons.append(f"test is not called by run_all_tests: {test}")

        diagnostics = rule.get("diagnostics")
        if not isinstance(diagnostics, list):
            reasons.append("diagnostics must be an array")
        else:
            for code in diagnostics:
                if code not in known_codes:
                    reasons.append(f"diagnostic is not registered: {code}")

        failures.extend(f"{rule_id}: {reason}" for reason in reasons)
        results.append({"id": rule_id, "status": "fail" if reasons else "pass", "reasons": reasons})

    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps({
        "schema": "vitte.type-system-rule-check",
        "schema_version": "1.0.0",
        "status": "fail" if failures else "pass",
        "rule_count": len(rules),
        "results": results,
        "failures": failures,
    }, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"[type-system-rules] status={'fail' if failures else 'pass'} rules={len(rules)} report={REPORT.relative_to(ROOT)}")
    for message in failures:
        print(f"[type-system-rules][error] {message}", file=sys.stderr)
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
