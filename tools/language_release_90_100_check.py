#!/usr/bin/env python3
from __future__ import annotations

import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "target" / "reports" / "language_release_90_100.json"


REQS = [
    (90, "official_linter", ["tools/vitte_lint.py", "Makefile"], ["Official Vitte linter", "vitte-lint"]),
    (91, "language_book_documentation", ["docs/book/index.html", "docs/book/chapters/04-syntax.html", "docs/book/chapters/05-types.html"], ["Vitte", "Syntax", "Types"]),
    (92, "beginner_tutorials", ["docs/book/poche/01-first-program.html", "docs/book/start-30-min.html", "examples"], ["first", "beginner"]),
    (93, "formal_specification", ["docs/spec/normative.md", "docs/spec/language.md", "docs/spec/type_system.md"], ["Judgments", "Memory Model", "Version Compatibility"]),
    (94, "conformance_tests", ["Makefile", "docs/book/grammar/scripts/validate_examples.py", "tests/grammar", "tests/diag_snapshots"], ["grammar-test", "core-language-gate"]),
    (95, "compiler_torture_suite", ["tools/compiler_stress_maximal.py", "tools/compiler_max_gate.sh", "tools/parser_lexer_fuzz_smoke.py"], ["stress", "fuzz", "crash"]),
    (96, "targeted_c_rust_zig_benchmarks", ["data/competitive_benchmarks/targeted_cases.json", "tools/public_benchmark_dashboard.py"], ["c", "rust", "zig"]),
    (97, "release_policy", ["docs/compiler/release_engineering.md", "docs/release/one_zero_policy.md", "tools/release_doctor.py"], ["release", "checksums"]),
    (98, "language_stdlib_semver", ["docs/release/version_contract.json", "docs/release/compatibility_matrix.md", "tools/version_compatibility_gate.py"], ["language_semver", "stdlib_semver"]),
    (99, "contribution_governance", ["docs/CONTRIBUTING.html", "docs/governance/platform_governance_complete.md"], ["contributing", "RFC"]),
    (100, "one_zero_green_gate_policy", ["docs/release/one_zero_policy.md", ".github/workflows/ci.yml", ".github/workflows/bootstrap.yml"], ["selfhost-hard-strict", "diagnostic-quality", "stdlib-gate", "backend-gate", "1.0"]),
]


def read_evidence(paths: list[str]) -> str:
    chunks: list[str] = []
    for item in paths:
        path = ROOT / item
        if path.is_file():
            chunks.append(path.read_text(encoding="utf-8", errors="replace"))
        elif path.is_dir():
            chunks.append("\n".join(str(p.relative_to(ROOT)) for p in path.rglob("*") if p.is_file()))
    return "\n".join(chunks)


def semver_ok() -> list[str]:
    contract = ROOT / "docs" / "release" / "version_contract.json"
    if not contract.exists():
        return ["missing version contract"]
    data = json.loads(contract.read_text(encoding="utf-8"))
    failures: list[str] = []
    pattern = re.compile(r"^[0-9]+\.[0-9]+\.[0-9]+(?:[-+][0-9A-Za-z.-]+)?$")
    for key in ("language_semver", "stdlib_semver"):
        value = str(data.get(key, ""))
        if not pattern.match(value):
            failures.append(f"{key} is not semver: {value}")
    return failures


def main() -> int:
    failures: list[str] = []
    rows: list[dict[str, object]] = []
    for req_id, name, evidence, needles in REQS:
        missing = [item for item in evidence if not (ROOT / item).exists()]
        text = read_evidence(evidence)
        absent = [needle for needle in needles if needle.lower() not in text.lower()]
        covered = not missing and not absent
        if missing:
            failures.append(f"{req_id} {name}: missing evidence {', '.join(missing)}")
        if absent:
            failures.append(f"{req_id} {name}: missing proof terms {', '.join(absent)}")
        rows.append({
            "id": req_id,
            "name": name,
            "evidence": evidence,
            "required_terms": needles,
            "missing_evidence": missing,
            "missing_terms": absent,
            "covered": covered,
        })

    for failure in semver_ok():
        failures.append(f"98 language_stdlib_semver: {failure}")

    payload = {
        "schema": "vitte.language_release_90_100",
        "schema_version": "1.0.0",
        "status": "pass" if not failures else "fail",
        "requirements": rows,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    if failures:
        for failure in failures:
            print(f"[language-release-90-100][error] {failure}", file=sys.stderr)
        print(f"[language-release-90-100] report={REPORT.relative_to(ROOT)}", file=sys.stderr)
        return 1
    print(f"[language-release-90-100] OK requirements=90-100 report={REPORT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
