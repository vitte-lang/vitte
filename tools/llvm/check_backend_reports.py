#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
REPORTS = ROOT / "target" / "reports"

VALIDATION_JSON = REPORTS / "llvm_backend_validation.json"
COVERAGE_JSON = REPORTS / "llvm_backend_coverage.json"

REQUIRED_FEATURES = [
    "llvm_ir_emission",
    "object_generation",
    "debug_info",
    "lto",
    "thin_lto",
    "pgo",
    "multi_arch",
]


def load_json(path: Path) -> dict:
    if not path.exists():
        raise SystemExit(f"[llvm-report][error] missing report: {path.relative_to(ROOT)}")
    return json.loads(path.read_text(encoding="utf-8"))


def main() -> int:
    validation = load_json(VALIDATION_JSON)
    coverage = load_json(COVERAGE_JSON)

    summary = validation.get("summary", {})
    if summary.get("status") != "PASS":
        print("[llvm-report][error] validation report is not PASS")
        return 1
    if summary.get("missing_files") != 0 or summary.get("missing_symbols") != 0:
        print("[llvm-report][error] validation report has missing files or symbols")
        return 1

    features = coverage.get("features", {})
    missing_features = [name for name in REQUIRED_FEATURES if features.get(name) is not True]
    if missing_features:
        print("[llvm-report][error] coverage report is missing required enabled features:")
        for name in missing_features:
            print(f"  - {name}")
        return 1

    artifacts = coverage.get("artifacts", {})
    ir_path = ROOT / str(artifacts.get("llvm_ir", ""))
    if not ir_path.exists() or not artifacts.get("llvm_ir_sha256"):
        print("[llvm-report][error] coverage report does not point to a hashed LLVM IR artifact")
        return 1

    print(
        "[llvm-report] "
        f"features={len(REQUIRED_FEATURES)} "
        "validation=PASS "
        "coverage=PASS"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
