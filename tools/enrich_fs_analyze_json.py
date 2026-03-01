#!/usr/bin/env python3
from __future__ import annotations

import json
import time
from pathlib import Path


def percentile(values: list[float], q: float) -> float:
    if not values:
        return 0.0
    xs = sorted(values)
    idx = int(round((len(xs) - 1) * q))
    return float(xs[max(0, min(len(xs) - 1, idx))])


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    report = repo / "target/reports/fs_analyze.json"
    telemetry = repo / ".vitte-cache/vitte-ide-gtk/telemetry.log"
    payload: dict = {}

    if report.exists():
        raw = report.read_text(encoding="utf-8").strip()
        try:
            payload = json.loads(raw) if raw else {}
        except json.JSONDecodeError:
            payload = {"raw": raw}

    lat_ms: list[float] = []
    if telemetry.exists():
        for line in telemetry.read_text(encoding="utf-8", errors="ignore").splitlines():
            if "|find_in_files|" not in line:
                continue
            for frag in line.split("|")[-1].split(","):
                if frag.startswith("ms="):
                    try:
                        lat_ms.append(float(frag[3:]))
                    except ValueError:
                        pass

    payload["fs_telemetry"] = {
        "generated_at": int(time.time()),
        "latency_p50_ms": percentile(lat_ms, 0.5),
        "latency_p95_ms": percentile(lat_ms, 0.95),
    }
    report.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
