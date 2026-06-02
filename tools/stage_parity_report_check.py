#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "target" / "reports" / "stage_parity" / "stage1_stage2_parity.json"
DEFAULT_MANIFEST = ROOT / "tools" / "stage_parity_sources.txt"


def die(message: str) -> int:
    print(f"[stage-parity-report][error] {message}")
    return 1


def read_manifest(path: Path) -> list[str]:
    items: list[str] = []
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        items.append(line)
    return items


def main() -> int:
    if not REPORT.is_file():
        return die(f"missing report: {REPORT.relative_to(ROOT)}")

    data = json.loads(REPORT.read_text(encoding="utf-8"))
    if data.get("schema") != "vitte.stage_parity.report":
        return die("unexpected schema")
    if data.get("schema_version") != "1.0.0":
        return die("unexpected schema_version")

    manifest_rel = data.get("source_manifest")
    if not isinstance(manifest_rel, str) or not manifest_rel:
        return die("missing source_manifest")

    manifest_path = ROOT / manifest_rel
    if not manifest_path.is_file():
        return die(f"missing manifest: {manifest_rel}")

    expected_sources = read_manifest(manifest_path)
    report_sources = data.get("sources")
    if not isinstance(report_sources, list):
        return die("sources must be a list")

    actual_sources = [item.get("source") for item in report_sources if isinstance(item, dict)]
    if actual_sources != expected_sources:
        return die("report sources do not match manifest order/content")

    if data.get("source_count") != len(expected_sources):
        return die("source_count does not match manifest")
    if data.get("ok_count") != len(expected_sources):
        return die("ok_count does not match manifest")

    native_json = data.get("native_json")
    if not isinstance(native_json, dict):
        return die("native_json must be an object")
    enabled_surface_count = native_json.get("enabled_surface_count")
    if enabled_surface_count != 4:
        return die("expected 4 enabled native JSON surfaces")

    for item in report_sources:
        if not isinstance(item, dict):
            return die("invalid source entry")
        if item.get("match") is not True:
            return die(f"source mismatch for {item.get('source', '<unknown>')}")
        for phase in ("parse", "check", "ir"):
            if item.get(phase, {}).get("match") is not True:
                return die(f"{phase} mismatch for {item.get('source', '<unknown>')}")
        surfaces = item.get("native_json", {}).get("surfaces", {})
        for name in ("ast", "hir", "mir", "diagnostics"):
            surface = surfaces.get(name, {})
            if surface.get("available") is not True:
                return die(f"{name} surface unavailable for {item.get('source', '<unknown>')}")
            if surface.get("match") is not True:
                return die(f"{name} surface mismatch for {item.get('source', '<unknown>')}")

    if manifest_path != DEFAULT_MANIFEST:
        print(f"[stage-parity-report] ok report={REPORT.relative_to(ROOT)} manifest={manifest_rel}")
    else:
        print(f"[stage-parity-report] ok report={REPORT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
