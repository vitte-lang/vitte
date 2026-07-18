#!/usr/bin/env python3

from __future__ import annotations

import hashlib
import json
import platform
import re
import sys
from dataclasses import asdict, dataclass
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]

REPORT_DIR = ROOT / "target" / "reports"
STDLIB_DIR = ROOT / "target" / "stdlib"
SOURCE_STDLIB_DIR = ROOT / "src/vitte/stdlib"
HISTORY_DIR = STDLIB_DIR / "history"

REPORT_DIR.mkdir(parents=True, exist_ok=True)
STDLIB_DIR.mkdir(parents=True, exist_ok=True)
HISTORY_DIR.mkdir(parents=True, exist_ok=True)

JSON_REPORT = REPORT_DIR / "stdlib_validation.json"
MARKDOWN_REPORT = REPORT_DIR / "stdlib_validation.md"
HTML_REPORT = REPORT_DIR / "stdlib_validation.html"


REQUIRED_FILES = [
    ROOT / "src/vitte/stdlib/mod.vit",
    ROOT / "src/vitte/stdlib/tests/smoke.vit",
]


REQUIRED_SYMBOLS = [
    "vector_new",
    "vector_push",
    "vector_pop",
    "vector_len",
    "hashmap_new",
    "hashmap_insert",
    "hashmap_get",
    "hashmap_remove",
    "io_read_file",
    "io_write_file",
]


OPTIONAL_SYMBOLS = [
    "async_spawn",
    "async_sleep",
    "channel_send",
    "channel_recv",
    "ffi_call",
    "http_get",
    "http_post",
    "json_parse",
    "json_stringify",
    "regex_match",
    "regex_replace",
]


FEATURE_MODULES = {
    "collections": "collections.vitl",
    "vector": "collections/vector.vitl",
    "hashmap": "collections/hashmap.vitl",
    "hashset": "collections/hashset.vitl",
    "deque": "collections/deque.vitl",
    "io": "io.vitl",
    "filesystem": "io/file.vitl",
    "async": "async.vitl",
    "channels": "async/channel.vitl",
    "ffi": "ffi/ffi.vitl",
    "network": "network.vitl",
    "http": "network/http.vitl",
    "json": "json.vitl",
    "yaml": "yaml.vitl",
    "toml": "toml.vitl",
    "regex": "regex.vitl",
    "crypto": "crypto.vitl",
    "compression": "compression.vitl",
    "image": "image.vitl",
    "audio": "audio.vitl",
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


def stdlib_symbol_index() -> dict[str, str]:
    symbols: dict[str, str] = {}
    declaration = re.compile(r"^\s*proc\s+([A-Za-z_][A-Za-z0-9_]*)\b")
    sources = sorted(SOURCE_STDLIB_DIR.glob("**/*.vit*"))

    for source_path in sources:
        for line in source_path.read_text(encoding="utf-8", errors="ignore").splitlines():
            match = declaration.match(line)
            if match and match.group(1) not in symbols:
                symbols[match.group(1)] = str(source_path.relative_to(ROOT))

    return symbols


def validate_required_symbols(
    symbols: dict[str, str],
) -> list[ValidationResult]:

    out = []

    for symbol in REQUIRED_SYMBOLS:
        out.append(
            ValidationResult(
                name=symbol,
                status=symbol in symbols,
                detail=symbols.get(symbol, "missing declaration"),
            )
        )

    return out


def validate_optional_symbols(
    symbols: dict[str, str],
) -> list[ValidationResult]:

    out = []

    for symbol in OPTIONAL_SYMBOLS:
        out.append(
            ValidationResult(
                name=symbol,
                status=symbol in symbols,
                detail=symbols.get(symbol, "missing declaration"),
            )
        )

    return out


def detected_features() -> dict[str, bool]:
    return {
        name: (SOURCE_STDLIB_DIR / relative_path).is_file()
        for name, relative_path in FEATURE_MODULES.items()
    }


def feature_score(features: dict[str, bool]) -> float:
    enabled = sum(
        1
        for value in features.values()
        if value
    )

    return round(
        enabled * 100.0
        / len(features),
        2,
    )


def build_report() -> dict:
    symbols = stdlib_symbol_index()
    features = detected_features()

    files = validate_files()

    required = validate_required_symbols(
        symbols
    )

    optional = validate_optional_symbols(
        symbols
    )

    missing_files = [
        item
        for item in files
        if not item.status
    ]

    missing_symbols = [
        item
        for item in required
        if not item.status
    ]

    status = (
        "PASS"
        if not missing_files
        and not missing_symbols
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
        "status": status,
        "coverage_score":
            feature_score(features),
        "features":
            features,
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
                len(missing_files),
            "missing_symbols":
                len(missing_symbols),
        },
    }


def write_json(report: dict):
    JSON_REPORT.write_text(
        json.dumps(
            report,
            indent=2,
            ensure_ascii=False,
        ) + "\n",
        encoding="utf-8",
    )


def write_markdown(report: dict):
    lines = [
        "# Standard Library Validation",
        "",
        f"Status: **{report['status']}**",
        "",
        f"Coverage: {report['coverage_score']}%",
        "",
        "## Required Files",
        "",
        "| File | Status |",
        "|------|------|",
    ]

    for item in report["required_files"]:
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

    for item in report["required_symbols"]:
        lines.append(
            f"| {item['name']} | "
            f"{'PASS' if item['status'] else 'FAIL'} |"
        )

    MARKDOWN_REPORT.write_text(
        "\n".join(lines) + "\n",
        encoding="utf-8",
    )


def write_html(report: dict):
    rows = []

    for item in report["required_symbols"]:
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
<title>Vitte Stdlib Validation</title>
</head>
<body>

<h1>Vitte Standard Library Validation</h1>

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


def save_history(report: dict):
    stamp = datetime.now(
        timezone.utc
    ).strftime(
        "%Y%m%dT%H%M%SZ"
    )

    history = (
        HISTORY_DIR
        / f"stdlib_validation_{stamp}.json"
    )

    history.write_text(
        json.dumps(
            report,
            indent=2,
        ) + "\n",
        encoding="utf-8",
    )


def main() -> int:
    report = build_report()

    write_json(report)
    write_markdown(report)
    write_html(report)

    save_history(report)

    print(
        f"[stdlib] status: "
        f"{report['status']}"
    )

    print(
        f"[stdlib] coverage: "
        f"{report['coverage_score']}%"
    )

    print(
        f"[stdlib] json: "
        f"{JSON_REPORT}"
    )

    if report["status"] != "PASS":
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
