#!/usr/bin/env python3
from __future__ import annotations

import json
import statistics
import csv
import html
from pathlib import Path
from datetime import datetime, timezone

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build" / "examples"
HISTORY = BUILD / "history"

REGRESSIONS_JSON = BUILD / "regressions.json"
REGRESSIONS_MD = BUILD / "regressions.md"
REGRESSION_HTML = BUILD / "regression.html"
REGRESSIONS_CSV = BUILD / "regressions.csv"

CRITICAL_THRESHOLD = 50
WARNING_THRESHOLD = 20
WATCH_THRESHOLD = 5
TOP_COUNT = 50
OUTPUT_VERSION = "v2"

REGRESSION_THRESHOLD = 20.0

COMPILER_PIPELINE = [
    "lexer",
    "parser",
    "ast",
    "hir",
    "sema",
    "typeck",
    "borrowck",
    "mir",
    "ir",
    "backend",
    "linker",
]

ROADMAP = [
    "parallel_examples_execution",
    "historical_trend_analysis",
    "automatic_regression_detection",
    "interactive_dashboard",
    "compiler_stage_benchmarks",
    "stdlib_benchmarks",
    "knowledge_graph_integration",
    "docs_performance_portal",
]


def load_json(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as f:
        return json.load(f)


def severity(delta: float) -> str:
    if delta >= CRITICAL_THRESHOLD:
        return "critical"
    if delta >= WARNING_THRESHOLD:
        return "warning"
    if delta >= WATCH_THRESHOLD:
        return "watch"
    return "stable"


def category_from_file(path: str) -> str:
    parts = path.split("/")
    if len(parts) >= 2:
        return parts[0]
    return "other"


def build_index(report: dict) -> dict:
    out = {}

    for item in report.get("results", []):
        out[item["file"]] = item

    return out


def find_snapshots() -> tuple[Path | None, Path | None]:
    HISTORY.mkdir(parents=True, exist_ok=True)

    snapshots = sorted(HISTORY.glob("*.json"))

    if not snapshots:
        return None, None

    if len(snapshots) == 1:
        return None, snapshots[0]

    return snapshots[-2], snapshots[-1]


def build_timing_stats(report: dict) -> dict:
    values = []

    for item in report.get("results", []):
        value = int(item.get("elapsed_ms", 0))
        if value > 0:
            values.append(value)

    if not values:
        return {
            "count": 0,
            "average_ms": 0,
            "median_ms": 0,
            "min_ms": 0,
            "max_ms": 0,
            "total_elapsed_ms": 0,
        }

    return {
        "count": len(values),
        "average_ms": round(sum(values) / len(values), 2),
        "median_ms": round(statistics.median(values), 2),
        "min_ms": min(values),
        "max_ms": max(values),
        "total_elapsed_ms": sum(values),
    }


def compute_regressions(previous: dict, current: dict) -> list[dict]:
    previous_index = build_index(previous)
    current_index = build_index(current)

    regressions = []

    for file_name, current_item in current_index.items():
        if file_name not in previous_index:
            continue

        old_ms = int(previous_index[file_name].get("elapsed_ms", 0))
        new_ms = int(current_item.get("elapsed_ms", 0))

        if old_ms <= 0:
            continue

        delta = ((new_ms - old_ms) * 100.0) / old_ms

        if delta <= 0:
            continue

        regressions.append({
            "file": file_name,
            "old_ms": old_ms,
            "new_ms": new_ms,
            "delta_ms": new_ms - old_ms,
            "delta_percent": round(delta, 2),
            "category": category_from_file(file_name),
            "severity": severity(delta),
        })

    regressions.sort(
        key=lambda item: item["delta_percent"],
        reverse=True,
    )

    return regressions[:TOP_COUNT]


def build_summary(regressions: list[dict], current: dict) -> dict:
    critical = sum(1 for r in regressions if r["severity"] == "critical")
    warning = sum(1 for r in regressions if r["severity"] == "warning")

    avg = 0.0

    if regressions:
        avg = sum(r["delta_percent"] for r in regressions) / len(regressions)

    worst = max([0.0] + [r["delta_percent"] for r in regressions])

    current_stats = build_timing_stats(current)
    slowest_example_ms = current_stats["max_ms"]

    return {
        "total_examples": current.get("total", 0),
        "regressions_detected": len(regressions),
        "critical_regressions": critical,
        "warning_regressions": warning,
        "average_delta_percent": round(avg, 2),
        "worst_delta_percent": round(worst, 2),
        "average_ms": current_stats["average_ms"],
        "median_ms": current_stats["median_ms"],
        "min_ms": current_stats["min_ms"],
        "max_ms": current_stats["max_ms"],
        "total_elapsed_ms": current_stats["total_elapsed_ms"],
        "slowest_example_ms": slowest_example_ms,
        "regression_threshold_percent": REGRESSION_THRESHOLD,
    }


def build_categories(regressions: list[dict]) -> dict:
    categories = {}

    for item in regressions:
        cat = item["category"]

        if cat not in categories:
            categories[cat] = {
                "regressions": 0,
                "worst_delta_percent": 0,
            }

        categories[cat]["regressions"] += 1
        categories[cat]["worst_delta_percent"] = max(
            categories[cat]["worst_delta_percent"],
            item["delta_percent"],
        )

    return categories


def build_severity_stats(regressions: list[dict]) -> dict:
    return {
        "critical": sum(1 for r in regressions if r["severity"] == "critical"),
        "warning": sum(1 for r in regressions if r["severity"] == "warning"),
        "watch": sum(1 for r in regressions if r["severity"] == "watch"),
        "stable": sum(1 for r in regressions if r["severity"] == "stable"),
    }


def write_csv_report(data: dict):
    with REGRESSIONS_CSV.open("w", encoding="utf-8", newline="") as f:
        writer = csv.writer(f)

        writer.writerow([
            "file",
            "old_ms",
            "new_ms",
            "delta_ms",
            "delta_percent",
            "severity",
            "category",
        ])

        for item in data.get("top_regressions", []):
            writer.writerow([
                item["file"],
                item["old_ms"],
                item["new_ms"],
                item["delta_ms"],
                item["delta_percent"],
                item["severity"],
                item["category"],
            ])


def write_json_report(data: dict):
    REGRESSIONS_JSON.write_text(
        json.dumps(data, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )


def write_markdown_report(data: dict):
    lines = []

    lines.append("# Vitte Performance Regressions")
    lines.append("")
    lines.append(f"Generated: {datetime.now(timezone.utc).isoformat()}")
    lines.append("")
    lines.append("## Summary")
    lines.append("")

    summary = data["summary"]

    for key, value in summary.items():
        lines.append(f"- **{key}**: {value}")

    lines.append("")
    lines.append("## Performance")
    lines.append("")

    perf = data.get("performance", {})
    for key, value in perf.items():
        lines.append(f"- **{key}**: {value}")

    lines.append("")
    lines.append("## Severity Distribution")
    lines.append("")

    for key, value in data.get("severity", {}).items():
        lines.append(f"- **{key}**: {value}")

    lines.append("")
    lines.append("## Compiler Pipeline")
    lines.append("")

    for stage in data.get("compiler_pipeline", []):
        lines.append(f"- {stage}")

    lines.append("")
    lines.append("## Roadmap")
    lines.append("")

    for step in data.get("roadmap", {}).get("next_steps", []):
        lines.append(f"- {step}")

    lines.append("")
    lines.append("## Top Regressions")
    lines.append("")
    lines.append("| Example | Old | New | Delta ms | Delta % | Severity |")
    lines.append("|----------|----------|----------|----------|----------|----------|")

    for item in data["top_regressions"]:
        lines.append(
            f"| {item['file']} | {item['old_ms']}ms | {item['new_ms']}ms | +{item['delta_ms']} | +{item['delta_percent']}% | {item['severity']} |"
        )

    REGRESSIONS_MD.write_text(
        "\n".join(lines) + "\n",
        encoding="utf-8",
    )


def write_html_report(data: dict):
    rows = []

    for item in data["top_regressions"]:
        rows.append(
            f"<tr><td>{html.escape(str(item['file']))}</td><td>{item['old_ms']}ms</td><td>{item['new_ms']}ms</td><td>{item['delta_percent']}%</td><td>{html.escape(str(item['severity']))}</td></tr>"
        )

    summary = data["summary"]

    pipeline_html = "".join(
        f"<li>{stage}</li>"
        for stage in data.get("compiler_pipeline", [])
    )

    roadmap_html = "".join(
        f"<li>{step}</li>"
        for step in data.get("roadmap", {}).get("next_steps", [])
    )

    quality_html = "".join(
        f"<li><strong>{k}</strong>: {v}</li>"
        for k, v in data.get("quality", {}).items()
    )

    severity_html = "".join(
        f"<li><strong>{html.escape(str(k))}</strong>: {v}</li>"
        for k, v in data.get("severity", {}).items()
    )

    category_rows = []

    for name, info in data.get("categories", {}).items():
        category_rows.append(
            f"<tr><td>{html.escape(str(name))}</td><td>{info['regressions']}</td><td>{info['worst_delta_percent']}</td></tr>"
        )

    html = f"""
<!doctype html>
<html>
<head>
<meta charset='utf-8'>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<title>Vitte Regressions</title>
<style>
  body {{
    background-color: #121212;
    color: #e0e0e0;
    font-family: Arial, sans-serif;
    padding: 1rem;
  }}
  table {{
    border-collapse: collapse;
    width: 100%;
    margin-bottom: 1rem;
  }}
  th, td {{
    border: 1px solid #444;
    padding: 0.5rem;
    text-align: left;
  }}
  th {{
    background-color: #1e1e1e;
    position: sticky;
    top: 0;
  }}
  tr:nth-child(even) {{
    background-color: #1a1a1a;
  }}
  h1, h2 {{
    color: #f0f0f0;
  }}
  ul {{
    list-style-type: none;
    padding-left: 0;
  }}
  ul li {{
    padding: 0.2rem 0;
  }}
</style>
</head>
<body>
<h1>Vitte Performance Regressions</h1>
<h2>Summary</h2>
<ul>
<li>Total Examples: {summary['total_examples']}</li>
<li>Regressions: {summary['regressions_detected']}</li>
<li>Average ms: {summary['average_ms']}</li>
<li>Median ms: {summary['median_ms']}</li>
<li>Worst Delta %: {summary['worst_delta_percent']}</li>
</ul>

<h2>Severity Distribution</h2>
<ul>
{severity_html}
</ul>

<h2>Quality</h2>
<ul>
{quality_html}
</ul>

<h2>Categories</h2>
<table>
<thead>
<tr>
<th>Category</th>
<th>Regressions</th>
<th>Worst Delta %</th>
</tr>
</thead>
<tbody>
{''.join(category_rows)}
</tbody>
</table>

<h2>Compiler Pipeline</h2>
<ul>
{pipeline_html}
</ul>

<h2>Roadmap</h2>
<ul>
{roadmap_html}
</ul>
<table border='1'>
<tr>
<th>Example</th>
<th>Old</th>
<th>New</th>
<th>Delta</th>
<th>Severity</th>
</tr>
{''.join(rows)}
</table>
</body>
</html>
"""

    REGRESSION_HTML.write_text(html, encoding="utf-8")


def build_quality_report(regressions: list[dict], categories: dict) -> dict:
    critical = sum(1 for r in regressions if r["severity"] == "critical")
    warning = sum(1 for r in regressions if r["severity"] == "warning")

    largest_category = "none"
    largest_count = 0

    for name, info in categories.items():
        if info["regressions"] > largest_count:
            largest_category = name
            largest_count = info["regressions"]

    return {
        "total_categories": len(categories),
        "largest_category": largest_category,
        "largest_category_regressions": largest_count,
        "critical_regression_count": critical,
        "warning_regression_count": warning,
        "ci_ready": True,
    }


def main():
    previous_path, current_path = find_snapshots()

    if current_path is None:
        print("No benchmark history found.")
        print(f"Create at least one snapshot in: {HISTORY}")
        return

    current = load_json(current_path)

    if previous_path is None:
        regressions = []
        categories = {}
    else:
        previous = load_json(previous_path)
        regressions = compute_regressions(previous, current)
        categories = build_categories(regressions)

    severity_stats = build_severity_stats(regressions)
    summary = build_summary(regressions, current)

    threshold_exceeded = any(
        item["delta_percent"] >= REGRESSION_THRESHOLD
        for item in regressions
    )

    report = {
        "version": OUTPUT_VERSION,
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "summary": summary,
        "top_regressions": regressions,
        "categories": categories,
        "severity": severity_stats,
        "performance": {
            "average_ms": summary["average_ms"],
            "median_ms": summary["median_ms"],
            "min_ms": summary["min_ms"],
            "max_ms": summary["max_ms"],
            "total_elapsed_ms": summary["total_elapsed_ms"],
            "slowest_example_ms": summary["slowest_example_ms"],
        },
        "quality": build_quality_report(regressions, categories),
        "compiler_pipeline": COMPILER_PIPELINE,
        "roadmap": {
            "next_steps": ROADMAP,
        },
        "threshold": REGRESSION_THRESHOLD,
        "threshold_exceeded": threshold_exceeded,
        "history": {
            "baseline": previous_path.name if previous_path else None,
            "current": current_path.name,
        },
    }

    write_json_report(report)
    write_csv_report(report)
    write_markdown_report(report)
    write_html_report(report)

    print(f"regressions: {len(regressions)}")
    print(f"json: {REGRESSIONS_JSON}")
    print(f"csv: {REGRESSIONS_CSV}")
    print(f"markdown: {REGRESSIONS_MD}")
    print(f"html: {REGRESSION_HTML}")
    print(f"threshold exceeded: {threshold_exceeded}")
    print(f"pipeline stages: {len(COMPILER_PIPELINE)}")
    print(f"roadmap items: {len(ROADMAP)}")


if __name__ == "__main__":
    main()
