#!/usr/bin/env python3
from __future__ import annotations
import csv
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
DATA = ROOT / "data" / "beta_feedback"
REQUIRED_FILES = [DATA / f"week-{i}.csv" for i in range(1, 5)]
REQUIRED_COLUMNS = [
    "tester_id",
    "profile",
    "loc_written",
    "time_to_productivity_hours",
    "pain_points_count",
    "critical_bugs_count",
    "core_api_sentiment_positive",
    "real_world_project_progress",
]
ALLOWED_PROGRESS = {"none", "in-progress", "completed"}


def fail(msg: str) -> int:
    print(f"[validate-feedback][error] {msg}", file=sys.stderr)
    return 1


def validate_file(path: Path) -> int:
    if not path.exists():
        return fail(f"missing required file: {path.relative_to(ROOT)}")
    with path.open(newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        cols = reader.fieldnames or []
        if cols != REQUIRED_COLUMNS:
            return fail(f"invalid columns in {path.name}: {cols}")
        row_count = 0
        for idx, row in enumerate(reader, start=2):
            row_count += 1
            if not row["tester_id"].strip():
                return fail(f"{path.name}:{idx} empty tester_id")
            try:
                int(row["loc_written"])
                float(row["time_to_productivity_hours"])
                int(row["pain_points_count"])
                int(row["critical_bugs_count"])
            except ValueError:
                return fail(f"{path.name}:{idx} invalid numeric field")
            if row["core_api_sentiment_positive"].strip().lower() not in {"true", "false"}:
                return fail(f"{path.name}:{idx} invalid boolean core_api_sentiment_positive")
            if row["real_world_project_progress"].strip() not in ALLOWED_PROGRESS:
                return fail(f"{path.name}:{idx} invalid real_world_project_progress")
        if row_count == 0:
            return fail(f"{path.name} has no data rows")
    print(f"[validate-feedback] ok: {path.name}")
    return 0


def main() -> int:
    for p in REQUIRED_FILES:
        rc = validate_file(p)
        if rc != 0:
            return rc
    print("[validate-feedback] all required weekly CSV files are valid")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
