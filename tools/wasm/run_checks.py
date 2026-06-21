#!/usr/bin/env python3

from __future__ import annotations

import hashlib
import json
import platform
import sys
from dataclasses import asdict, dataclass
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]

REPORT_DIR = ROOT / "target" / "reports"
WASM_DIR = ROOT / "target" / "wasm"

REPORT_DIR.mkdir(parents=True, exist_ok=True)
WASM_DIR.mkdir(parents=True, exist_ok=True)

JSON_REPORT = REPORT_DIR / "wasm_validation.json"
MARKDOWN_REPORT = REPORT_DIR / "wasm_validation.md"
HTML_REPORT = REPORT_DIR / "wasm_validation.html"

REQUIRED_FILES = [
    ROOT / "src/vitte/compiler/backends/wasm/mod.vit",
    ROOT / "src/vitte/compiler/backends/wasm/tests/smoke.vit",
    ROOT / "src/vitte/compiler/backends/vitte_emit/mod.vit",
]

REQUIRED_SYMBOLS = [
    "emit_wat",
    "emit_wasm",
    "wasi_enabled",
    "web_api_surface",
    "size_optimization_mode",
]

OPTIONAL_SYMBOLS = [
    "emit_source_map",
    "emit_debug_info",
    "emit_component_model",
    "emit_gc_metadata",
    "emit_simd",
    "emit_threads",
    "emit_exceptions",
]

FEATURES = {
    "wat_emission": True,
    "wasm_emission": True,
    "wasi_support": True,
    "browser_support": True,
    "fetch_api": True,
    "console_api": True,
    "timers_api": True,
    "size_optimization": True,
    "source_maps": True,
    "debug_info": True,
    "simd": False,
    "threads": False,
    "exceptions": False,
    "gc": False,
    "component_model": False,
}


@dataclass
class ValidationResult:
    name: str
    status: bool
    detail: str


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()

    with path.open("rb") as f:
        while chunk := f.read(1024 * 1024):
            h.update(chunk)

    return h.hexdigest()


def validate_files() -> list[ValidationResult]:
    results = []

    for file in REQUIRED_FILES:
        results.append(
            ValidationResult(
                name=str(file.relative_to(ROOT)),
                status=file.exists(),
                detail=(
                    sha256_file(file)
                    if file.exists()
                    else "missing"
                ),
            )
        )

    return results


def load_backend_source() -> str:
    backend = (
        ROOT
        / "src/vitte/compiler/backends/wasm/mod.vit"
    )

    if not backend.exists():
        return ""

    return backend.read_text(
        encoding="utf-8",
        errors="ignore",
    )


def validate_symbols(
    source: str,
) -> list[ValidationResult]:

    results = []

    for symbol in REQUIRED_SYMBOLS:
        results.append(
            ValidationResult(
                name=symbol,
                status=symbol in source,
                detail="required",
            )
        )

    return results


def validate_optional_symbols(
    source: str,
) -> list[ValidationResult]:

    results = []

    for symbol in OPTIONAL_SYMBOLS:
        results.append(
            ValidationResult(
                name=symbol,
                status=symbol in source,
                detail="optional",
            )
        )

    return results


def feature_score() -> float:
    enabled = sum(
        1
        for value in FEATURES.values()
        if value
    )

    return round(
        enabled * 100.0
        / len(FEATURES),
        2,
    )


def build_report() -> dict:
    source = load_backend_source()

    files = validate_files()

    required = validate_symbols(
        source
    )

    optional = validate_optional_symbols(
        source
    )

    failed_files = [
        item
        for item in files
        if not item.status
    ]

    failed_required = [
        item
        for item in required
        if not item.status
    ]

    status = (
        "PASS"
        if not failed_files
        and not failed_required
        else "FAIL"
    )

    return {
        "version": "v3",
        "generated_at":
            datetime.now(
                timezone.utc
            ).isoformat(),
        "host": {
            "platform":
                platform.platform(),
            "machine":
                platform.machine(),
            "python":
                platform.python_version(),
        },
        "backend": "wasm",
        "status": status,
        "coverage_score":
            feature_score(),
        "features": FEATURES,
        "required_files": [
            asdict(item)
            for item in files
        ],
        "required_symbols": [
            asdict(item)
            for item in required
        ],
        "optional_symbols": [
            asdict(item)
            for item in optional
        ],
        "statistics": {
            "required_files":
                len(files),
            "required_symbols":
                len(required),
            "optional_symbols":
                len(optional),
            "missing_files":
                len(failed_files),
            "missing_symbols":
                len(failed_required),
        },
    }


def write_json(report: dict):
    JSON_REPORT.write_text(
        json.dumps(
            report,
            indent=2,
            ensure_ascii=False,
        )
        + "\n",
        encoding="utf-8",
    )


def write_markdown(report: dict):
    lines = [
        "# WASM Backend Validation",
        "",
        f"Status: **{report['status']}**",
        "",
        f"Coverage: "
        f"{report['coverage_score']}%",
        "",
        "## Required Files",
        "",
        "| File | Status |",
        "|------|------|",
    ]

    for item in report[
        "required_files"
    ]:
        lines.append(
            f"| {item['name']} | "
            f"{'PASS' if item['status'] else 'FAIL'} |"
        )

    lines.extend([
        "",
        "## Required Symbols",
        "",
        "| Symbol | Status |",
        "|------|------|",
    ])

    for item in report[
        "required_symbols"
    ]:
        lines.append(
            f"| {item['name']} | "
            f"{'PASS' if item['status'] else 'FAIL'} |"
        )

    MARKDOWN_REPORT.write_text(
        "\n".join(lines)
        + "\n",
        encoding="utf-8",
    )


def write_html(report: dict):
    rows = []

    for item in report[
        "required_symbols"
    ]:
        rows.append(
            f"<tr>"
            f"<td>{item['name']}</td>"
            f"<td>{'PASS' if item['status'] else 'FAIL'}</td>"
            f"</tr>"
        )

    html = f"""
<!doctype html>
<html>
<head>
<meta charset="utf-8">
<title>WASM Backend Validation</title>
</head>
<body>

<h1>WASM Backend Validation</h1>

<p>Status: {report["status"]}</p>
<p>Coverage: {report["coverage_score"]}%</p>

<table border="1">
<tr>
<th>Symbol</th>
<th>Status</th>
</tr>

{''.join(rows)}

</table>

</body>
</html>
"""

    HTML_REPORT.write_text(
        html,
        encoding="utf-8",
    )


def main() -> int:
    report = build_report()

    write_json(report)
    write_markdown(report)
    write_html(report)

    print(
        f"[wasm] status: "
        f"{report['status']}"
    )

    print(
        f"[wasm] coverage: "
        f"{report['coverage_score']}%"
    )

    print(
        f"[wasm] json: "
        f"{JSON_REPORT}"
    )

    if report["status"] != "PASS":
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
