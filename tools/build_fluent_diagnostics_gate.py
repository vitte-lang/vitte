#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "bin" / "vitte"
REPORT = ROOT / "target" / "reports" / "build_fluent_diagnostics" / "coverage.json"
OUTPUT_ROOT = ROOT / "target" / "build_fluent_diagnostics"
VALID_SOURCE = "target/build_fluent_diagnostics/valid.vit"
ESCAPED_MISSING_SOURCE = "target/build_fluent_diagnostics/missing-\"quote\"-backslash\\-tab\t-unicode-\u00e9-newline\n.vit"
RAW_ERROR_MARKER = "[vitte][error]"
RAW_IO_MARKER = "E_CLI_IO: cannot read"
ANSI_RE = re.compile(r"\x1b\[[0-9;]*m")
TEXT_CODE_RE = re.compile(r"(?:error|fatal)\[([A-Z0-9_]+)\]")


@dataclass(frozen=True)
class Case:
    case_id: str
    argv: tuple[str, ...]
    expected_codes: tuple[str, ...]
    expected_span: str = ""
    env_overrides: tuple[tuple[str, str], ...] = ()


def fixture(path: str) -> str:
    return path


CORE_CASES = (
    Case("build-output-required", ("build", VALID_SOURCE), ("E_CLI_MISSING_ARG",), "<cli>"),
    Case("file-missing", ("build", "target/fixtures/does-not-exist.vit", "-o", "target/build_fluent_diagnostics/missing"), ("E_IO_FILE_NOT_FOUND",), "target/fixtures/does-not-exist.vit"),
    Case("output-value-missing", ("build", VALID_SOURCE, "-o"), ("E_CLI_MISSING_ARG",), "<cli>"),
    Case("output-directory-missing", ("build", VALID_SOURCE, "-o", "target/build_fluent_diagnostics/not-a-directory/out"), ("E_IO_DIRECTORY_NOT_FOUND",), "target/build_fluent_diagnostics/not-a-directory/out"),
    Case("output-overwrites-source", ("build", VALID_SOURCE, "-o", VALID_SOURCE), ("E_IO_OVERWRITE_FORBIDDEN",), VALID_SOURCE),
    Case("unknown-option", ("build", VALID_SOURCE, "--not-a-vitte-option"), ("E_CLI_UNKNOWN_OPTION",)),
    Case("lexer-invalid", ("build", fixture("tests/compiler_real_diagnostics/invalid/lexer/lexer_invalid.vit"), "-o", "target/build_fluent_diagnostics/lexer"), ("LEX_E_INVALID_CHAR",)),
    Case("parser-invalid", ("build", fixture("tests/compiler_real_diagnostics/invalid/parser/parser_invalid.vit"), "-o", "target/build_fluent_diagnostics/parser"), ("PARSE_E_UNCLOSED_BLOCK",)),
    Case("import-missing", ("build", fixture("tests/compiler_real_diagnostics/invalid/resolver/use_import_invalid.vit"), "-o", "target/build_fluent_diagnostics/import"), ("MOD_E_MODULE_NOT_FOUND",)),
    Case("symbol-unknown", ("build", fixture("tests/compiler_real_diagnostics/invalid/sema/unknown_symbol.vit"), "-o", "target/build_fluent_diagnostics/symbol"), ("SEMA_E_UNKNOWN_IDENTIFIER",)),
    Case("type-mismatch", ("build", fixture("tests/compiler_real_diagnostics/invalid/typeck/types_incompatible.vit"), "-o", "target/build_fluent_diagnostics/type"), ("TYPECK_E_ASSIGN_MISMATCH",)),
    Case("return-mismatch", ("build", fixture("tests/compiler_real_diagnostics/invalid/typeck/return_invalid.vit"), "-o", "target/build_fluent_diagnostics/return"), ("TYPECK_E_RETURN_MISMATCH",)),
    Case("borrow-invalid", ("build", fixture("tests/compiler_real_diagnostics/invalid/borrowck/borrowck_use_after_move.vit"), "-o", "target/build_fluent_diagnostics/borrow"), ("BORROWCK_E_USE_AFTER_MOVE",)),
    Case("mir-invalid", ("build", fixture("tests/compiler_real_diagnostics/invalid/mir/mir_validation_invalid.vit"), "-o", "target/build_fluent_diagnostics/mir"), ("MIR_E_VERIFICATION_FAILED",)),
    Case("ir-invalid", ("build", fixture("tests/compiler_real_diagnostics/invalid/ir/ir_validation_invalid.vit"), "-o", "target/build_fluent_diagnostics/ir"), ("IR_E_VERIFY_FAILED",)),
    Case("backend-unsupported", ("build", VALID_SOURCE, "-o", "target/build_fluent_diagnostics/backend", "--target", "unsupported-target"), ("BACKEND_E_UNSUPPORTED_TARGET",)),
    Case("linker-output-not-materialized", ("build", VALID_SOURCE, "-o", "target/build_fluent_diagnostics/linker-missing", "--no-native"), ("LINK_E_OUTPUT_NOT_MATERIALIZED",), "target/build_fluent_diagnostics/linker-missing"),
)

EXTENDED_CASES = (
    Case("unknown-command", ("not-a-command",), ("E_CLI_UNKNOWN_COMMAND",), "<cli>"),
    Case("unexpected-positional", ("build", VALID_SOURCE, "extra.vit", "-o", "target/build_fluent_diagnostics/extra"), ("E_CLI_INVALID_ARGUMENT",), "<cli>"),
    Case("lang-value-missing", ("build", VALID_SOURCE, "-o", "target/build_fluent_diagnostics/lang", "--lang"), ("E_CLI_MISSING_ARG",), "<cli>"),
    Case("target-value-missing", ("build", VALID_SOURCE, "-o", "target/build_fluent_diagnostics/target", "--target"), ("E_CLI_MISSING_ARG",), "<cli>"),
    Case("profile-value-missing", ("build", VALID_SOURCE, "-o", "target/build_fluent_diagnostics/profile", "--profile"), ("E_CLI_MISSING_ARG",), "<cli>"),
    Case("input-value-missing", ("build", "-o", "target/build_fluent_diagnostics/input", "--input"), ("E_CLI_MISSING_ARG",), "<cli>"),
    Case("empty-input-path", ("build", "--input", "", "-o", "target/build_fluent_diagnostics/empty-input"), ("E_CLI_MISSING_ARG",), "<cli>"),
    Case("diagnostic-format-value-missing", ("build", VALID_SOURCE, "-o", "target/build_fluent_diagnostics/format-missing", "--format"), ("E_CLI_MISSING_ARG",), "<cli>"),
    Case("diagnostic-format-invalid", ("build", VALID_SOURCE, "-o", "target/build_fluent_diagnostics/format", "--format", "yaml"), ("E_CLI_INVALID_ARGUMENT",), "<cli>"),
    Case("error-format-invalid", ("build", VALID_SOURCE, "-o", "target/build_fluent_diagnostics/error-format", "--error-format=xml"), ("E_CLI_INVALID_ARGUMENT",), "<cli>"),
    Case("color-value-missing", ("build", VALID_SOURCE, "-o", "target/build_fluent_diagnostics/color-missing", "--color"), ("E_CLI_MISSING_ARG",), "<cli>"),
    Case("color-value-invalid", ("build", VALID_SOURCE, "-o", "target/build_fluent_diagnostics/color", "--color=rainbow"), ("E_CLI_INVALID_ARGUMENT",), "<cli>"),
    Case("counterfactual-level-value-missing", ("build", VALID_SOURCE, "-o", "target/build_fluent_diagnostics/counterfactual-missing", "--counterfactual-level"), ("E_CLI_MISSING_ARG",), "<cli>"),
    Case("counterfactual-level-invalid", ("build", VALID_SOURCE, "-o", "target/build_fluent_diagnostics/counterfactual", "--counterfactual-level", "impossible"), ("E_CLI_INVALID_ARGUMENT",), "<cli>"),
    Case("target-empty-equals", ("build", VALID_SOURCE, "-o", "target/build_fluent_diagnostics/target-empty", "--target="), ("E_CLI_MISSING_ARG",), "<cli>"),
    Case("profile-empty-equals", ("build", VALID_SOURCE, "-o", "target/build_fluent_diagnostics/profile-empty", "--profile="), ("E_CLI_MISSING_ARG",), "<cli>"),
    Case("input-not-regular", ("build", "target/build_fluent_diagnostics/source-directory", "-o", "target/build_fluent_diagnostics/source-directory-out"), ("E_IO_FILE_UNREADABLE",), "target/build_fluent_diagnostics/source-directory"),
    Case("input-file-unreadable", ("build", "target/build_fluent_diagnostics/unreadable.vit", "-o", "target/build_fluent_diagnostics/unreadable-out"), ("E_IO_FILE_UNREADABLE",), "target/build_fluent_diagnostics/unreadable.vit"),
    Case("output-parent-missing", ("build", VALID_SOURCE, "-o", "target/build_fluent_diagnostics/missing-parent/out"), ("E_IO_DIRECTORY_NOT_FOUND",), "target/build_fluent_diagnostics/missing-parent/out"),
    Case("output-is-directory", ("build", VALID_SOURCE, "-o", "target/build_fluent_diagnostics/output-directory"), ("E_IO_OUTPUT_UNWRITABLE",), "target/build_fluent_diagnostics/output-directory"),
    Case("output-unwritable", ("build", VALID_SOURCE, "-o", "target/build_fluent_diagnostics/unwritable/out"), ("E_IO_OUTPUT_UNWRITABLE",), "target/build_fluent_diagnostics/unwritable/out"),
    Case("output-symlink-overwrites-source", ("build", "target/build_fluent_diagnostics/symlink-source.vit", "-o", "target/build_fluent_diagnostics/symlink-output"), ("E_IO_OVERWRITE_FORBIDDEN",), "target/build_fluent_diagnostics/symlink-output"),
    Case("backend-missing-c-compiler", ("build", VALID_SOURCE, "-o", "target/build_fluent_diagnostics/missing-compiler"), ("BACKEND_E_MISSING_C_COMPILER",), "target/build_fluent_diagnostics/missing-compiler", (("PATH", "target/build_fluent_diagnostics/no-tools"),)),
    Case("pkg-subcommand-unknown", ("pkg", "not-a-subcommand"), ("E_CLI_UNKNOWN_COMMAND",), "<cli>"),
    Case("option-terminator", ("check", "--", "--not-an-option"), ("E_IO_FILE_NOT_FOUND",), "--not-an-option"),
    Case("duplicate-output-last-wins", ("build", VALID_SOURCE, "-o", "target/build_fluent_diagnostics/missing-parent/ignored", "-o", "target/build_fluent_diagnostics/duplicate-output", "--no-native"), ("LINK_E_OUTPUT_NOT_MATERIALIZED",), "target/build_fluent_diagnostics/duplicate-output"),
    Case("native-options-incompatible", ("build", VALID_SOURCE, "-o", "target/build_fluent_diagnostics/native-conflict", "--native", "--no-native"), ("E_CLI_INVALID_ARGUMENT",), "<cli>"),
    Case("empty-output-path", ("build", VALID_SOURCE, "-o", ""), ("E_CLI_MISSING_ARG",), "<cli>"),
    Case("structured-escaping", ("build", ESCAPED_MISSING_SOURCE, "-o", "target/build_fluent_diagnostics/escaped"), ("E_IO_FILE_NOT_FOUND",), ESCAPED_MISSING_SOURCE),
)

CASES = CORE_CASES + EXTENDED_CASES


def command(case: Case, surface: str) -> list[str]:
    argv = [str(BIN), case.argv[0], "--lang", "fr", *case.argv[1:]]
    if surface == "json":
        argv.insert(2, "--diagnostics-json")
    elif surface == "lsp":
        argv.insert(2, "--diagnostics-lsp")
    return argv


def run(case: Case, surface: str) -> dict[str, Any]:
    reset_case_fixture(case)
    proc = subprocess.run(
        command(case, surface),
        cwd=ROOT,
        env={**os.environ, "VITTE_LANG": "fr", **dict(case.env_overrides)},
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    return {
        "argv": ["bin/vitte", *command(case, surface)[1:]],
        "exit_code": proc.returncode,
        "stdout": proc.stdout,
        "stderr": proc.stderr,
    }


def required_diagnostic_fields(value: dict[str, Any]) -> list[str]:
    missing: list[str] = []
    for key in ("code", "id", "category", "severity", "fluent_key", "message", "span", "labels", "cause", "help", "fix", "example", "suggestions"):
        if value.get(key) in (None, "", []):
            missing.append(key)
    return missing


def validate_text(case: Case, result: dict[str, Any]) -> list[str]:
    failures: list[str] = []
    output = result["stdout"] + result["stderr"]
    if result["exit_code"] == 0:
        failures.append("invalid build exited with zero")
    if RAW_ERROR_MARKER in output:
        failures.append("raw [vitte][error] output is forbidden")
    if RAW_IO_MARKER in output:
        failures.append("raw E_CLI_IO text is forbidden")
    if "fatal[" in output:
        failures.append("public terminal diagnostics must use error[CODE]")
    if ANSI_RE.search(output):
        failures.append("terminal diagnostic contains ANSI escapes")
    codes = tuple(TEXT_CODE_RE.findall(output))
    if codes != case.expected_codes:
        failures.append(f"diagnostic order mismatch: expected {case.expected_codes}, got {codes}")
    for marker in ("= id:", "= category:", "= severity:", "= fluent-key:", "= span:", "= label:", "= cause:", "= help:", "= fix-it:", "= corrected example:"):
        if marker not in output:
            failures.append(f"missing rich text marker {marker}")
    if case.expected_span and f"= span: {case.expected_span}:" not in output:
        failures.append(f"text diagnostic span does not identify {case.expected_span}")
    return failures


def validate_json(case: Case, result: dict[str, Any]) -> list[str]:
    failures: list[str] = []
    if result["exit_code"] == 0:
        failures.append("invalid JSON build exited with zero")
    if result["stderr"]:
        failures.append("JSON diagnostic wrote non-JSON stderr")
    try:
        payload = json.loads(result["stdout"])
    except json.JSONDecodeError as exc:
        return [f"JSON output is not parseable: {exc}"]
    diagnostics = payload.get("primary_report", {}).get("diagnostics", [])
    codes = tuple(item.get("code") for item in diagnostics if isinstance(item, dict))
    if codes != case.expected_codes:
        failures.append(f"JSON diagnostic order mismatch: expected {case.expected_codes}, got {codes}")
    for index, value in enumerate(diagnostics):
        if not isinstance(value, dict):
            failures.append(f"JSON diagnostic {index} is not an object")
            continue
        missing = required_diagnostic_fields(value)
        if missing:
            failures.append(f"JSON diagnostic {index} missing fields: {', '.join(missing)}")
    if case.expected_span and diagnostics:
        span = diagnostics[0].get("span", {})
        if not isinstance(span, dict) or span.get("file") != case.expected_span:
            failures.append(f"JSON diagnostic span does not identify {case.expected_span}")
    return failures


def validate_lsp(case: Case, result: dict[str, Any]) -> list[str]:
    failures: list[str] = []
    if result["exit_code"] == 0:
        failures.append("invalid LSP build exited with zero")
    if result["stderr"]:
        failures.append("LSP diagnostic wrote non-JSON stderr")
    try:
        payload = json.loads(result["stdout"])
    except json.JSONDecodeError as exc:
        return [f"LSP output is not parseable JSON: {exc}"]
    if payload.get("jsonrpc") != "2.0" or payload.get("method") != "textDocument/publishDiagnostics":
        failures.append("LSP output is not a publishDiagnostics JSON-RPC notification")
    diagnostics = payload.get("params", {}).get("diagnostics", [])
    codes = tuple(item.get("code") for item in diagnostics if isinstance(item, dict))
    if codes != case.expected_codes:
        failures.append(f"LSP diagnostic order mismatch: expected {case.expected_codes}, got {codes}")
    for index, value in enumerate(diagnostics):
        if not isinstance(value, dict):
            failures.append(f"LSP diagnostic {index} is not an object")
            continue
        for key in ("range", "severity", "code", "codeDescription", "source", "message", "relatedInformation", "data"):
            if value.get(key) in (None, "", []):
                failures.append(f"LSP diagnostic {index} missing {key}")
        data = value.get("data", {})
        if not isinstance(data, dict):
            failures.append(f"LSP diagnostic {index} data is not an object")
            continue
        for key in ("code", "id", "category", "severity", "fluent_key", "message", "span", "labels", "cause", "help", "fix", "example", "suggestions", "code_action_title"):
            if data.get(key) in (None, "", []):
                failures.append(f"LSP diagnostic {index} data missing {key}")
    if case.expected_span and diagnostics:
        data = diagnostics[0].get("data", {})
        span = data.get("span", {}) if isinstance(data, dict) else {}
        if not isinstance(span, dict) or span.get("file") != case.expected_span:
            failures.append(f"LSP diagnostic span does not identify {case.expected_span}")
    return failures


def runtime_state() -> str:
    if not BIN.exists():
        return "binary-missing"
    data = BIN.read_bytes()
    if RAW_ERROR_MARKER.encode() in data or RAW_IO_MARKER.encode() in data:
        return "runtime-obsolete"
    if b"_command_build" in data and b"run_cli_main" not in data:
        return "runtime-obsolete"
    return "runtime-current"


def write_blocker_report(state: str, blocker: str) -> None:
    report = {
        "schema": "vitte.build-fluent-diagnostics.coverage.v1",
        "binary": "bin/vitte",
        "locale": "fr",
        "runtime_state": state,
        "core_case_count": len(CORE_CASES),
        "case_count": len(CASES),
        "passed_count": 0,
        "failure_count": 1,
        "cases": [],
        "status": "failed",
        "blocker": blocker,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def prepare_fixtures() -> None:
    OUTPUT_ROOT.mkdir(parents=True, exist_ok=True)
    (ROOT / VALID_SOURCE).write_text("space diagnostics/matrix\nproc main() -> int { give 0; }\n", encoding="utf-8")
    output_blocker = OUTPUT_ROOT / "not-a-directory"
    if output_blocker.is_dir():
        shutil.rmtree(output_blocker)
    output_blocker.write_text("blocks output directory creation\n", encoding="utf-8")
    shutil.rmtree(OUTPUT_ROOT / "missing-parent", ignore_errors=True)
    (OUTPUT_ROOT / "source-directory").mkdir(exist_ok=True)
    (OUTPUT_ROOT / "output-directory").mkdir(exist_ok=True)
    (OUTPUT_ROOT / "no-tools").mkdir(exist_ok=True)
    unreadable = OUTPUT_ROOT / "unreadable.vit"
    unreadable.write_text("space diagnostics/unreadable\nproc main() -> int { give 0 }\n", encoding="utf-8")
    unreadable.chmod(0)
    unwritable = OUTPUT_ROOT / "unwritable"
    unwritable.mkdir(exist_ok=True)
    unwritable.chmod(0o500)


def reset_case_fixture(case: Case) -> None:
    if case.case_id == "output-overwrites-source":
        (ROOT / VALID_SOURCE).write_text("space diagnostics/matrix\nproc main() -> int { give 0; }\n", encoding="utf-8")
    elif case.case_id == "output-symlink-overwrites-source":
        source = OUTPUT_ROOT / "symlink-source.vit"
        output = OUTPUT_ROOT / "symlink-output"
        source.write_text("space diagnostics/symlink\nproc main() -> int { give 0 }\n", encoding="utf-8")
        output.unlink(missing_ok=True)
        output.symlink_to(source.resolve())
    elif case.case_id == "output-unwritable":
        directory = OUTPUT_ROOT / "unwritable"
        directory.chmod(0o700)
        (directory / "out").unlink(missing_ok=True)
        directory.chmod(0o500)
    elif case.case_id == "linker-output-not-materialized":
        (OUTPUT_ROOT / "linker-missing").unlink(missing_ok=True)


def restore_fixtures() -> None:
    unreadable = OUTPUT_ROOT / "unreadable.vit"
    unwritable = OUTPUT_ROOT / "unwritable"
    if unreadable.exists():
        unreadable.chmod(0o600)
    if unwritable.exists():
        unwritable.chmod(0o700)
    valid_source = ROOT / VALID_SOURCE
    if valid_source.exists():
        valid_source.write_text("space diagnostics/matrix\nproc main() -> int { give 0; }\n", encoding="utf-8")
    (OUTPUT_ROOT / "symlink-output").unlink(missing_ok=True)


def validate_empty_lsp() -> dict[str, Any]:
    proc = subprocess.run(
        [str(BIN), "check", "--diagnostics-lsp", "tests/pipeline/hello_world.vit", "--lang", "fr"],
        cwd=ROOT,
        env={**os.environ, "VITTE_LANG": "fr"},
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    failures: list[str] = []
    payload: dict[str, Any] = {}
    if proc.returncode != 0:
        failures.append(f"valid LSP check exited with {proc.returncode}")
    if proc.stderr:
        failures.append("valid LSP check wrote stderr")
    try:
        payload = json.loads(proc.stdout)
    except json.JSONDecodeError as exc:
        failures.append(f"empty LSP notification is not parseable: {exc}")
    if payload:
        if payload.get("jsonrpc") != "2.0" or payload.get("method") != "textDocument/publishDiagnostics":
            failures.append("empty LSP output is not publishDiagnostics")
        if payload.get("params", {}).get("diagnostics") != []:
            failures.append("valid LSP check did not publish an empty diagnostic list")
    return {
        "argv": ["bin/vitte", "check", "--diagnostics-lsp", "tests/pipeline/hello_world.vit", "--lang", "fr"],
        "exit_code": proc.returncode,
        "stdout": proc.stdout,
        "stderr": proc.stderr,
        "failures": failures,
        "passed": not failures,
    }


def main() -> int:
    state = runtime_state()
    if not BIN.exists():
        write_blocker_report(state, "runtime compiler artifact is missing")
        print(
            f"[build-fluent-diagnostics-gate][error] missing runtime compiler artifact: {BIN.relative_to(ROOT)}; report={REPORT.relative_to(ROOT)}",
            file=sys.stderr,
        )
        return 1
    try:
        prepare_fixtures()
    except OSError as exc:
        restore_fixtures()
        write_blocker_report(state, f"fixture setup failed: {exc}")
        print(f"[build-fluent-diagnostics-gate][error] fixture setup failed: {exc}", file=sys.stderr)
        return 1
    try:
        rows: list[dict[str, Any]] = []
        failure_count = 1 if state != "runtime-current" else 0
        validators = {"text": validate_text, "json": validate_json, "lsp": validate_lsp}
        for case in CASES:
            surfaces: dict[str, Any] = {}
            for surface, validator in validators.items():
                result = run(case, surface)
                failures = validator(case, result)
                failure_count += len(failures)
                surfaces[surface] = {**result, "failures": failures, "passed": not failures}
            rows.append({
                "id": case.case_id,
                "expected_codes": list(case.expected_codes),
                "surfaces": surfaces,
                "passed": all(value["passed"] for value in surfaces.values()),
            })

        empty_lsp = validate_empty_lsp()
        failure_count += len(empty_lsp["failures"])

        report = {
            "schema": "vitte.build-fluent-diagnostics.coverage.v1",
            "binary": "bin/vitte",
            "locale": "fr",
            "runtime_state": state,
            "core_case_count": len(CORE_CASES),
            "case_count": len(CASES),
            "passed_count": sum(1 for row in rows if row["passed"]),
            "failure_count": failure_count,
            "cases": rows,
            "empty_lsp": empty_lsp,
            "status": "ok" if failure_count == 0 and state == "runtime-current" else "failed",
        }
        if state == "runtime-obsolete":
            report["blocker"] = "runtime compiler artifact does not execute the current Vitte driver"
        REPORT.parent.mkdir(parents=True, exist_ok=True)
        REPORT.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    finally:
        restore_fixtures()

    if failure_count:
        print(
            f"[build-fluent-diagnostics-gate][error] passed={report['passed_count']}/{len(CASES)} failures={failure_count}; report={REPORT.relative_to(ROOT)}",
            file=sys.stderr,
        )
        return 1
    print(f"[build-fluent-diagnostics-gate] passed={len(CASES)}/{len(CASES)} report={REPORT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
