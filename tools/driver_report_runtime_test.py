#!/usr/bin/env python3
from __future__ import annotations

from collections import OrderedDict
from pathlib import Path
import json
import re
import sys

from frontend_syntax_check import parse_file


ROOT = Path(__file__).resolve().parent.parent
DRIVER_PIPELINE = ROOT / "src" / "vitte" / "compiler" / "driver" / "pipeline.vit"
FRONTEND_PIPELINE = ROOT / "src" / "vitte" / "compiler" / "frontend" / "pipeline.vit"
FIXTURE_DIR = ROOT / "tests" / "driver_report_runtime"


def extract_grammar_version() -> str:
    text = FRONTEND_PIPELINE.read_text(encoding="utf-8")
    match = re.search(r'grammar_version:\s*"([^"]+)"', text)
    if not match:
        raise RuntimeError("cannot extract grammar_version from frontend/pipeline.vit")
    return match.group(1)


def count_named_rows(proc_name: str) -> int:
    text = DRIVER_PIPELINE.read_text(encoding="utf-8")
    match = re.search(rf"proc {proc_name}\(\) -> \[string\] \{{(.*?)\n\}}", text, re.S)
    if not match:
        raise RuntimeError(f"cannot extract {proc_name} from driver/pipeline.vit")
    return len(re.findall(r'"[^"]+"', match.group(1)))


def extract_compile_report_fields() -> list[str]:
    text = DRIVER_PIPELINE.read_text(encoding="utf-8")
    match = re.search(r"form CompileReport \{(.*?)\n\}", text, re.S)
    if not match:
        raise RuntimeError("cannot extract CompileReport fields from driver/pipeline.vit")
    fields = []
    for raw in match.group(1).splitlines():
        line = raw.strip()
        if not line or ":" not in line:
            continue
        fields.append(line.split(":", 1)[0].strip())
    return fields


def extract_compile_report_json_keys() -> list[str]:
    text = DRIVER_PIPELINE.read_text(encoding="utf-8")
    match = re.search(r"proc compile_report_json\(report: CompileReport\) -> string \{(.*?)\n\}", text, re.S)
    if not match:
        raise RuntimeError("cannot extract compile_report_json from driver/pipeline.vit")
    return re.findall(r'\\"([a-z_]+)\\":', match.group(1))


def check_json_contract_drift() -> None:
    fields = extract_compile_report_fields()
    json_keys = extract_compile_report_json_keys()
    expected = []
    for field in fields:
        if field == "active_modules":
            expected.append("active_count")
        elif field == "inactive_modules":
            expected.append("inactive_count")
        else:
            expected.append(field)
    if set(json_keys) != set(expected) or len(json_keys) != len(set(json_keys)):
        raise AssertionError(
            "compile_report_json drift\n"
            f"expected keys: {sorted(set(expected))}\n"
            f"actual keys:   {sorted(set(json_keys))}"
        )


def driver_exit_code(command: str, source_text: str, parsed: dict[str, object]) -> int:
    if source_text == "":
        return 2
    if not parsed["valid"]:
        return 3
    if source_text == "borrowck:fail" or source_text == "const:fail":
        return 4
    if command not in {"compile", "check"}:
        return 1
    return 0


def main_driver_report_json(command: str, source_path: str, source_text: str) -> str:
    parsed = parse_file(ROOT / source_path)
    exit_code = driver_exit_code(command, source_text, parsed)
    report = OrderedDict(
        [
            ("command", command),
            ("source_path", source_path),
            ("exit_code", exit_code),
            ("status", "ok" if exit_code == 0 else "error"),
            ("grammar_version", extract_grammar_version()),
            ("token_count", parsed["token_count"]),
            ("toplevel_count", parsed["toplevel_count"]),
            ("stmt_count", parsed["stmt_count"]),
            ("expr_count", parsed["expr_count"]),
            ("type_count", parsed["type_count"]),
            ("pattern_count", parsed["pattern_count"]),
            ("active_count", count_named_rows("active_modules_table")),
            ("inactive_count", count_named_rows("inactive_modules_table")),
        ]
    )
    return json.dumps(report, separators=(",", ":"))


def check_case(
    command: str,
    fixture_name: str,
    snapshot_name: str,
    source_text_override: str | None = None,
) -> None:
    source_path = f"tests/driver_report_runtime/{fixture_name}"
    source_text = (FIXTURE_DIR / fixture_name).read_text(encoding="utf-8")
    if source_text_override is not None:
        source_text = source_text_override
    actual = main_driver_report_json(command, source_path, source_text)
    expected = (FIXTURE_DIR / snapshot_name).read_text(encoding="utf-8").strip()
    if actual != expected:
        raise AssertionError(
            f"{fixture_name} mismatch\nexpected: {expected}\nactual:   {actual}"
        )


def main() -> int:
    check_json_contract_drift()
    check_case("compile", "compile_fixture.vit", "compile_fixture.report.json")
    check_case("check", "check_fixture.vit", "check_fixture.report.json")
    check_case("check", "check_error_fixture.vit", "check_error_fixture.report.json")
    check_case(
        "check",
        "compile_fixture.vit",
        "borrowck_fail_fixture.report.json",
        source_text_override="borrowck:fail",
    )
    check_case(
        "check",
        "compile_fixture.vit",
        "const_fail_fixture.report.json",
        source_text_override="const:fail",
    )
    print("[driver-report-runtime-test] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
