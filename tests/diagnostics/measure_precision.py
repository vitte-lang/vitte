#!/usr/bin/env python3
"""Measure frontend diagnostic precision against checked-in fixture oracles."""

from __future__ import annotations

import argparse
import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from diagnostic_frontend import analyze  # noqa: E402


@dataclass(frozen=True, order=True)
class DiagnosticKey:
    code: str
    line: int
    column: int
    end_line: int
    end_column: int


@dataclass(frozen=True)
class PrecisionResult:
    fixtures: int
    true_positive: int
    false_positive: int
    false_negative: int

    @property
    def precision(self) -> float:
        denominator = self.true_positive + self.false_positive
        return 1.0 if denominator == 0 else self.true_positive / denominator

    @property
    def recall(self) -> float:
        denominator = self.true_positive + self.false_negative
        return 1.0 if denominator == 0 else self.true_positive / denominator


def expected_key(path: Path) -> DiagnosticKey:
    data = json.loads(path.read_text(encoding="utf-8"))
    return DiagnosticKey(
        code=data["code"],
        line=int(data["line"]),
        column=int(data["column"]),
        end_line=int(data["end_line"]),
        end_column=int(data["end_column"]),
    )


def actual_keys(source: Path) -> list[DiagnosticKey]:
    diagnostics = analyze(source.read_text(encoding="utf-8"), source.name)
    keys: list[DiagnosticKey] = []
    for diagnostic in diagnostics:
        if diagnostic.get("code") == "E_BOOTSTRAP_NATIVE_SUBSET":
            continue
        span = diagnostic["primary_span"]
        start = span["start"]
        end = span["end"]
        keys.append(
            DiagnosticKey(
                code=diagnostic["code"],
                line=int(start["line"]),
                column=int(start["column"]),
                end_line=int(end["line"]),
                end_column=int(end["column"]),
            )
        )
    return sorted(keys)


def measure(fixtures_root: Path) -> tuple[PrecisionResult, list[dict[str, Any]]]:
    sources = sorted(fixtures_root.glob("**/*.vit"))
    details: list[dict[str, Any]] = []
    true_positive = 0
    false_positive = 0
    false_negative = 0

    for source in sources:
        expect_path = source.with_suffix(".expect.json")
        if not expect_path.exists():
            raise SystemExit(f"missing diagnostic oracle: {expect_path}")

        expected = expected_key(expect_path)
        actual = actual_keys(source)
        matched = expected in actual
        extras = [key for key in actual if key != expected]

        if matched:
            true_positive += 1
        else:
            false_negative += 1
        false_positive += len(extras)

        details.append(
            {
                "fixture": str(source.relative_to(ROOT)),
                "expected": expected.__dict__,
                "actual": [key.__dict__ for key in actual],
                "matched": matched,
                "false_positive": [key.__dict__ for key in extras],
            }
        )

    return PrecisionResult(
        fixtures=len(sources),
        true_positive=true_positive,
        false_positive=false_positive,
        false_negative=false_negative,
    ), details


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--fixtures-root", type=Path, default=ROOT / "tests/diagnostics/frontend")
    parser.add_argument("--fail-under", type=float, default=1.0)
    parser.add_argument("--json-report", type=Path)
    args = parser.parse_args()

    result, details = measure(args.fixtures_root)
    payload = {
        "fixtures": result.fixtures,
        "true_positive": result.true_positive,
        "false_positive": result.false_positive,
        "false_negative": result.false_negative,
        "precision": result.precision,
        "recall": result.recall,
        "details": details,
    }
    if args.json_report is not None:
        args.json_report.parent.mkdir(parents=True, exist_ok=True)
        args.json_report.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    print(
        "diagnostic precision: "
        f"fixtures={result.fixtures} "
        f"true_positive={result.true_positive} "
        f"false_positive={result.false_positive} "
        f"false_negative={result.false_negative} "
        f"precision={result.precision:.2%} "
        f"recall={result.recall:.2%}"
    )
    if result.precision < args.fail_under:
        print(f"diagnostic precision below threshold: {result.precision:.4f} < {args.fail_under:.4f}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
