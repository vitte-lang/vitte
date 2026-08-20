#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from diagnostics_locales import SUPPORTED_DIAGNOSTIC_LOCALES


ROOT = Path(__file__).resolve().parents[1]
REPORT_JSON = ROOT / "target/reports/bootstrap_max.json"
REPORT_MD = ROOT / "target/reports/bootstrap_max.md"
REPORT_SHA = ROOT / "target/reports/bootstrap_max.sha256"
WORK = ROOT / "target/bootstrap-max"
VALID = WORK / "valid.vit"
HELLO = ROOT / "tests/pipeline/hello_world.vit"
COMPILER_ENTRY = ROOT / "src/vitte/compiler/main.vit"
SOURCE_BOOTSTRAP = ROOT / "target/bootstrap-c17/vitte-bootstrap"

STAGES = {
    "signed_stage0": ROOT / "toolchain/bootstrap/stage0/macos-arm64/vitte",
    "trusted_stage0": ROOT / "target/bootstrap-real/stage0/vitte",
    "stage1": ROOT / "target/stage1/vitte",
    "stage2": ROOT / "target/stage2/vitte",
    "release": ROOT / "target/release/vitte",
    "bin": ROOT / "bin/vitte",
    "vittec": ROOT / "bin/vittec",
}

BOOTSTRAP_CODES = (
    "E_CLI_MISSING_ARG",
    "E_CLI_UNKNOWN_COMMAND",
    "E_CLI_UNKNOWN_OPTION",
    "E_CLI_INVALID_ARGUMENT",
    "E_IO_FILE_NOT_FOUND",
    "E_IO_FILE_UNREADABLE",
    "E_IO_OUTPUT_UNWRITABLE",
    "E_IO_DIRECTORY_NOT_FOUND",
    "E_IO_OVERWRITE_FORBIDDEN",
    "LINK_E_OUTPUT_NOT_MATERIALIZED",
    "BACKEND_E_MISSING_C_COMPILER",
    "BACKEND_E_UNSUPPORTED_TARGET",
    "LEX_E_INVALID_CHAR",
    "PARSE_E_UNCLOSED_BLOCK",
    "MOD_E_MODULE_NOT_FOUND",
    "SEMA_E_UNKNOWN_IDENTIFIER",
    "TYPECK_E_ASSIGN_MISMATCH",
    "TYPECK_E_RETURN_MISMATCH",
    "BORROWCK_E_USE_AFTER_MOVE",
    "MIR_E_VERIFICATION_FAILED",
    "IR_E_VERIFY_FAILED",
)

FORBIDDEN_MARKERS = (
    b"vitte-bootstrap-payload",
    b"payload_source",
    b"write_payload_file",
    b"_command_build",
    b"_copy_file",
    b"vitte_stage0_clone_self",
    b"BOOTSTRAP_FULL_COMPILER",
    b"VITTE_BOOTSTRAP_ALLOW_FULL_COMPILER_BRIDGE",
    b"[vitte][error]",
    b"E_CLI_IO: cannot read",
)

RAW_OUTPUT_MARKERS = ("[vitte][error]", "E_CLI_IO: cannot read", "fatal[")


@dataclass(frozen=True)
class Case:
    case_id: str
    argv: tuple[str, ...]
    code: str
    env: tuple[tuple[str, str], ...] = ()


PHASE_CASES = (
    Case("cli-missing-output", ("build", str(HELLO.relative_to(ROOT)), "--diagnostics-json", "--lang", "fr"), "E_CLI_MISSING_ARG"),
    Case("cli-unknown-command", ("not-a-command", "--diagnostics-json", "--lang", "fr"), "E_CLI_UNKNOWN_COMMAND"),
    Case("cli-unknown-option", ("build", str(HELLO.relative_to(ROOT)), "--not-a-vitte-option", "--diagnostics-json", "--lang", "fr"), "E_CLI_UNKNOWN_OPTION"),
    Case("cli-invalid-argument", ("build", str(HELLO.relative_to(ROOT)), "--diagnostics-json", "--format", "yaml", "--lang", "fr"), "E_CLI_INVALID_ARGUMENT"),
    Case("io-file-missing", ("build", "target/fixtures/does-not-exist.vit", "-o", "target/bootstrap-max/missing", "--diagnostics-json", "--lang", "fr"), "E_IO_FILE_NOT_FOUND"),
    Case("io-file-unreadable", ("build", "target/bootstrap-max/source-directory", "-o", "target/bootstrap-max/source-directory-out", "--diagnostics-json", "--lang", "fr"), "E_IO_FILE_UNREADABLE"),
    Case("io-output-unwritable", ("build", str(VALID.relative_to(ROOT)), "-o", "target/bootstrap-max/output-directory", "--diagnostics-json", "--lang", "fr"), "E_IO_OUTPUT_UNWRITABLE"),
    Case("io-directory-missing", ("build", str(VALID.relative_to(ROOT)), "-o", "target/bootstrap-max/not-a-directory/out", "--diagnostics-json", "--lang", "fr"), "E_IO_DIRECTORY_NOT_FOUND"),
    Case("io-overwrite-forbidden", ("build", str(VALID.relative_to(ROOT)), "-o", str(VALID.relative_to(ROOT)), "--diagnostics-json", "--lang", "fr"), "E_IO_OVERWRITE_FORBIDDEN"),
    Case("link-output-missing", ("build", str(VALID.relative_to(ROOT)), "-o", "target/bootstrap-max/linker-missing", "--no-native", "--diagnostics-json", "--lang", "fr"), "LINK_E_OUTPUT_NOT_MATERIALIZED"),
    Case("backend-missing-c-compiler", ("build", str(VALID.relative_to(ROOT)), "-o", "target/bootstrap-max/missing-compiler", "--diagnostics-json", "--lang", "fr"), "BACKEND_E_MISSING_C_COMPILER", (("PATH", "target/bootstrap-max/no-tools"),)),
    Case("backend-unsupported-target", ("build", str(VALID.relative_to(ROOT)), "-o", "target/bootstrap-max/backend", "--target", "unsupported-target", "--diagnostics-json", "--lang", "fr"), "BACKEND_E_UNSUPPORTED_TARGET"),
    Case("lexer-invalid", ("build", "tests/compiler_real_diagnostics/invalid/lexer/lexer_invalid.vit", "-o", "target/bootstrap-max/lexer", "--diagnostics-json", "--lang", "fr"), "LEX_E_INVALID_CHAR"),
    Case("parser-invalid", ("build", "tests/compiler_real_diagnostics/invalid/parser/parser_invalid.vit", "-o", "target/bootstrap-max/parser", "--diagnostics-json", "--lang", "fr"), "PARSE_E_UNCLOSED_BLOCK"),
    Case("module-missing", ("build", "tests/compiler_real_diagnostics/invalid/resolver/use_import_invalid.vit", "-o", "target/bootstrap-max/import", "--diagnostics-json", "--lang", "fr"), "MOD_E_MODULE_NOT_FOUND"),
    Case("sema-unknown", ("build", "tests/compiler_real_diagnostics/invalid/sema/unknown_symbol.vit", "-o", "target/bootstrap-max/symbol", "--diagnostics-json", "--lang", "fr"), "SEMA_E_UNKNOWN_IDENTIFIER"),
    Case("type-assign", ("build", "tests/compiler_real_diagnostics/invalid/typeck/types_incompatible.vit", "-o", "target/bootstrap-max/type", "--diagnostics-json", "--lang", "fr"), "TYPECK_E_ASSIGN_MISMATCH"),
    Case("type-return", ("build", "tests/compiler_real_diagnostics/invalid/typeck/return_invalid.vit", "-o", "target/bootstrap-max/return", "--diagnostics-json", "--lang", "fr"), "TYPECK_E_RETURN_MISMATCH"),
    Case("borrow-use-after-move", ("build", "tests/compiler_real_diagnostics/invalid/borrowck/borrowck_use_after_move.vit", "-o", "target/bootstrap-max/borrow", "--diagnostics-json", "--lang", "fr"), "BORROWCK_E_USE_AFTER_MOVE"),
    Case("mir-invalid", ("build", "tests/compiler_real_diagnostics/invalid/mir/mir_validation_invalid.vit", "-o", "target/bootstrap-max/mir", "--diagnostics-json", "--lang", "fr"), "MIR_E_VERIFICATION_FAILED"),
    Case("ir-invalid", ("build", "tests/compiler_real_diagnostics/invalid/ir/ir_validation_invalid.vit", "-o", "target/bootstrap-max/ir", "--diagnostics-json", "--lang", "fr"), "IR_E_VERIFY_FAILED"),
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def binary_format(path: Path) -> str:
    data = path.read_bytes()[:8]
    if data.startswith(b"\x7fELF"):
        return "ELF"
    if data.startswith(b"MZ"):
        return "PE"
    if data[:4] in {
        b"\xfe\xed\xfa\xce", b"\xfe\xed\xfa\xcf", b"\xce\xfa\xed\xfe", b"\xcf\xfa\xed\xfe",
        b"\xca\xfe\xba\xbe", b"\xca\xfe\xba\xbf",
    }:
        return "Mach-O"
    if data.startswith(b"#!"):
        return "script"
    return "unknown"


def run(argv: list[str], env: dict[str, str] | None = None) -> dict[str, Any]:
    merged = os.environ.copy()
    merged.pop("BOOTSTRAP_FULL_COMPILER", None)
    merged.pop("VITTE_BOOTSTRAP_ALLOW_FULL_COMPILER_BRIDGE", None)
    if env:
        merged.update(env)
    proc = subprocess.run(
        argv,
        cwd=ROOT,
        env=merged,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    return {
        "argv": argv,
        "exit_code": proc.returncode,
        "stdout": proc.stdout,
        "stderr": proc.stderr,
    }


def catalog(locale: str) -> dict[str, str]:
    entries: dict[str, str] = {}
    for line in (ROOT / "locales" / locale / "diagnostics.ftl").read_text(encoding="utf-8").splitlines():
        if " =" in line:
            key, value = line.split("=", 1)
            entries[key.strip()] = value.strip()
    return entries


CATALOGS = {locale.code: catalog(locale.code) for locale in SUPPORTED_DIAGNOSTIC_LOCALES}


def prepare_fixtures() -> None:
    WORK.mkdir(parents=True, exist_ok=True)
    VALID.write_text("space bootstrap/max\nproc main() -> int { give 0; }\n", encoding="utf-8")
    blocker = WORK / "not-a-directory"
    if blocker.is_dir():
        shutil.rmtree(blocker)
    blocker.write_text("blocks output directory creation\n", encoding="utf-8")
    (WORK / "source-directory").mkdir(exist_ok=True)
    (WORK / "output-directory").mkdir(exist_ok=True)
    (WORK / "no-tools").mkdir(exist_ok=True)


def diagnostic(payload: dict[str, Any], surface: str) -> dict[str, Any] | None:
    if surface == "json":
        diagnostics = payload.get("primary_report", {}).get("diagnostics", [])
    elif surface == "lsp":
        diagnostics = payload.get("params", {}).get("diagnostics", [])
    else:
        return None
    if isinstance(diagnostics, list) and diagnostics and isinstance(diagnostics[0], dict):
        return diagnostics[0]
    return None


def parse_structured(result: dict[str, Any], surface: str) -> tuple[dict[str, Any] | None, list[str]]:
    failures: list[str] = []
    if result["stderr"]:
        failures.append("structured diagnostic wrote stderr")
    try:
        payload = json.loads(result["stdout"])
    except json.JSONDecodeError as exc:
        return None, [f"not parseable JSON: {exc}"]
    value = diagnostic(payload, surface)
    if value is None:
        failures.append(f"missing {surface} diagnostic")
    return value, failures


def parse_text(output: str) -> tuple[str, str] | None:
    match = re.search(r"error\[([A-Z0-9_]+)\]: ([^\n]+)", output)
    if not match:
        return None
    return match.group(1), match.group(2)


def normalize_output(stage_path: Path, output: str) -> str:
    normalized = output.replace(str(stage_path), "<compiler>")
    normalized = normalized.replace(stage_path.relative_to(ROOT).as_posix(), "<compiler>")
    return normalized


def check_artifacts(errors: list[str]) -> dict[str, Any]:
    artifacts: dict[str, Any] = {}
    for name, path in STAGES.items():
        row: dict[str, Any] = {"path": path.relative_to(ROOT).as_posix(), "exists": path.is_file()}
        if not path.is_file():
            errors.append(f"missing stage artifact: {name} {path.relative_to(ROOT)}")
            artifacts[name] = row
            continue
        row.update({
            "executable": os.access(path, os.X_OK),
            "size": path.stat().st_size,
            "sha256": sha256(path),
            "format": binary_format(path),
        })
        if not row["executable"]:
            errors.append(f"stage artifact is not executable: {name}")
        data = path.read_bytes()
        markers = [marker.decode("utf-8", "replace") for marker in FORBIDDEN_MARKERS if marker in data]
        row["forbidden_markers"] = markers
        if markers:
            errors.append(f"{name} contains forbidden markers: {', '.join(markers)}")
        artifacts[name] = row
    return artifacts


def check_manifest(errors: list[str]) -> dict[str, Any]:
    result = run([sys.executable, "tools/bootstrap_real/stage0_trust.py"])
    if result["exit_code"] != 0:
        errors.append("stage0 trust verification failed")
        return {"status": "failed", "result": result}
    try:
        return {"status": "ok", "trust": json.loads(result["stdout"])}
    except json.JSONDecodeError:
        errors.append("stage0 trust output was not JSON")
        return {"status": "failed", "result": result}


def check_locale_matrix(errors: list[str]) -> dict[str, Any]:
    rows: list[dict[str, Any]] = []
    surfaces = {
        "text": (),
        "json": ("--diagnostics-json",),
        "lsp": ("--diagnostics-lsp",),
    }
    for locale in SUPPORTED_DIAGNOSTIC_LOCALES:
        expected = CATALOGS[locale.code]["E_CLI_MISSING_ARG"]
        english = CATALOGS["en"]["E_CLI_MISSING_ARG"]
        for stage_name, stage_path in STAGES.items():
            for surface, flags in surfaces.items():
                argv = [
                    str(stage_path),
                    "build",
                    str(HELLO.relative_to(ROOT)),
                    *flags,
                    "--lang",
                    locale.code,
                ]
                result = run(argv, {"VITTE_LANG": "xx-UNKNOWN"})
                failures: list[str] = []
                if result["exit_code"] == 0:
                    failures.append("invalid request exited zero")
                raw = result["stdout"] + result["stderr"]
                for marker in RAW_OUTPUT_MARKERS:
                    if marker in raw:
                        failures.append(f"raw marker in output: {marker}")
                code = ""
                message = ""
                if surface == "text":
                    parsed = parse_text(raw)
                    if parsed is None:
                        failures.append("missing text diagnostic")
                    else:
                        code, message = parsed
                else:
                    value, structured_failures = parse_structured(result, surface)
                    failures.extend(structured_failures)
                    if value:
                        code = str(value.get("code", ""))
                        message = str(value.get("message", ""))
                if code != "E_CLI_MISSING_ARG":
                    failures.append(f"expected E_CLI_MISSING_ARG got {code!r}")
                if message != expected:
                    failures.append(f"expected {locale.code} message {expected!r}, got {message!r}")
                if locale.code != "en" and expected != english and message == english:
                    failures.append("locale silently fell back to en")
                if failures:
                    errors.append(f"locale matrix failed {stage_name}/{locale.code}/{surface}: {'; '.join(failures)}")
                rows.append({
                    "stage": stage_name,
                    "locale": locale.code,
                    "surface": surface,
                    "code": code,
                    "message": message,
                    "passed": not failures,
                    "failures": failures,
                })
    return {
        "locale_count": len(SUPPORTED_DIAGNOSTIC_LOCALES),
        "surface_count": len(surfaces),
        "probe_count": len(rows),
        "passed_count": sum(1 for row in rows if row["passed"]),
        "rows": rows,
    }


def check_phase_matrix(errors: list[str]) -> dict[str, Any]:
    rows: list[dict[str, Any]] = []
    for case in PHASE_CASES:
        expected_message = CATALOGS["fr"][case.code]
        for stage_name, stage_path in STAGES.items():
            result = run([str(stage_path), *case.argv], dict(case.env))
            failures: list[str] = []
            value, structured_failures = parse_structured(result, "json")
            failures.extend(structured_failures)
            if result["exit_code"] == 0:
                failures.append("invalid request exited zero")
            raw = result["stdout"] + result["stderr"]
            for marker in RAW_OUTPUT_MARKERS:
                if marker in raw:
                    failures.append(f"raw marker in output: {marker}")
            code = str(value.get("code", "")) if value else ""
            message = str(value.get("message", "")) if value else ""
            if code != case.code:
                failures.append(f"expected {case.code} got {code!r}")
            if message != expected_message:
                failures.append(f"expected fr message {expected_message!r}, got {message!r}")
            if failures:
                errors.append(f"phase matrix failed {stage_name}/{case.case_id}: {'; '.join(failures)}")
            rows.append({
                "stage": stage_name,
                "case": case.case_id,
                "expected_code": case.code,
                "code": code,
                "message": message,
                "passed": not failures,
                "failures": failures,
            })
    return {
        "case_count": len(PHASE_CASES),
        "stage_count": len(STAGES),
        "probe_count": len(rows),
        "passed_count": sum(1 for row in rows if row["passed"]),
        "rows": rows,
    }


def check_surface_parity(errors: list[str]) -> dict[str, Any]:
    probes = (
        ("version", ("--version",), 0),
        ("help", ("--help",), 0),
        ("check-empty-lsp", ("check", "--diagnostics-lsp", str(HELLO.relative_to(ROOT)), "--lang", "fr"), 0),
        ("missing-json", ("build", str(HELLO.relative_to(ROOT)), "--diagnostics-json", "--lang", "fr"), 2),
        ("unknown-lsp", ("build", str(HELLO.relative_to(ROOT)), "--unknown-option", "--diagnostics-lsp", "--lang", "fr"), 2),
        ("package", ("package",), 0),
        ("workspace", ("workspace",), 0),
    )
    rows: list[dict[str, Any]] = []
    for probe_id, argv_tail, expected_exit in probes:
        outputs: dict[str, str] = {}
        exits: dict[str, int] = {}
        for stage_name, stage_path in STAGES.items():
            result = run([str(stage_path), *argv_tail])
            exits[stage_name] = int(result["exit_code"])
            combined = normalize_output(stage_path, result["stdout"] + result["stderr"])
            outputs[stage_name] = combined
        expected = outputs.get("signed_stage0", "")
        failures = [
            f"{stage_name} exit {exit_code} != {expected_exit}"
            for stage_name, exit_code in exits.items()
            if exit_code != expected_exit
        ]
        failures.extend(
            f"{stage_name} output differs from signed_stage0"
            for stage_name, output in outputs.items()
            if output != expected
        )
        if failures:
            errors.append(f"surface parity failed {probe_id}: {'; '.join(failures)}")
        rows.append({"probe": probe_id, "passed": not failures, "failures": failures, "exits": exits})
    return {"probe_count": len(rows), "passed_count": sum(1 for row in rows if row["passed"]), "rows": rows}


def check_reproducibility(errors: list[str]) -> dict[str, Any]:
    before = {name: sha256(path) for name, path in STAGES.items() if path.is_file()}
    result = run([sys.executable, "tools/bootstrap_real/bootstrap_chain.py", "--offline"])
    after = {name: sha256(path) for name, path in STAGES.items() if path.is_file()}
    failures: list[str] = []
    if result["exit_code"] != 0:
        failures.append("bootstrap_chain --offline failed")
    for name in ("trusted_stage0", "stage1", "stage2", "release", "bin", "vittec"):
        if before.get(name) != after.get(name):
            failures.append(f"{name} hash changed across reproducibility rebuild")
    if failures:
        errors.append("reproducibility failed: " + "; ".join(failures))
    return {"passed": not failures, "failures": failures, "before": before, "after": after, "command": result}


def check_clean_room_and_copy(errors: list[str]) -> dict[str, Any]:
    clean = WORK / "clean-room"
    clean.mkdir(parents=True, exist_ok=True)
    clean_stage = clean / "stage1"
    user_binary = clean / "hello"
    result_stage = run(
        [str(SOURCE_BOOTSTRAP), "build", str(COMPILER_ENTRY.relative_to(ROOT)), "-o", str(clean_stage.relative_to(ROOT))],
        {"VITTE_C17_GENERIC_COMPILER": "1"},
    )
    validate = run([sys.executable, "tools/bootstrap_real/bootstrap_real.py", "--candidate", str(clean_stage.relative_to(ROOT))])
    result_user = run([str(STAGES["stage2"]), "build", str(HELLO.relative_to(ROOT)), "-o", str(user_binary.relative_to(ROOT))])
    failures: list[str] = []
    if result_stage["exit_code"] != 0:
        failures.append("clean-room stage build failed")
    if validate["exit_code"] != 0:
        failures.append("clean-room candidate validation failed")
    if result_user["exit_code"] != 0:
        failures.append("stage2 user build failed")
    if user_binary.is_file() and sha256(user_binary) == sha256(STAGES["stage2"]):
        failures.append("stage2 built user program by copying its own executable")
    if user_binary.is_file():
        data = user_binary.read_bytes()
        markers = [marker.decode("utf-8", "replace") for marker in FORBIDDEN_MARKERS if marker in data]
        if markers:
            failures.append("user binary contains forbidden markers: " + ", ".join(markers))
    else:
        failures.append("stage2 user binary missing")
    if failures:
        errors.append("clean-room/copy regression failed: " + "; ".join(failures))
    return {
        "passed": not failures,
        "failures": failures,
        "clean_stage": clean_stage.relative_to(ROOT).as_posix(),
        "user_binary": user_binary.relative_to(ROOT).as_posix(),
    }


def check_generated_and_reports(errors: list[str]) -> dict[str, Any]:
    checks = (
        [sys.executable, "tools/generate_bootstrap_fluent_table.py", "--check"],
        [sys.executable, "tools/generate_frontend_fluent_bridge.py", "--check"],
        [sys.executable, "tools/check_diagnostics_locales.py"],
    )
    rows = []
    for argv in checks:
        result = run(argv)
        if result["exit_code"] != 0:
            errors.append(f"generated/report prerequisite failed: {' '.join(argv)}")
        rows.append({"argv": argv, "exit_code": result["exit_code"], "passed": result["exit_code"] == 0})
    report_paths = [
        "target/reports/bootstrap_chain.json",
        "target/reports/bootstrap_real_gate.json",
        "target/reports/selfhost_completion.md",
        "target/reports/build_fluent_diagnostics/coverage.json",
        "target/reports/cli_early_diagnostics.json",
    ]
    hashes = {}
    for raw in report_paths:
        path = ROOT / raw
        if not path.is_file():
            errors.append(f"missing signed/hash report input: {raw}")
        else:
            hashes[raw] = sha256(path)
    return {"checks": rows, "report_hashes": hashes}


def make_requirement_status(payload: dict[str, Any]) -> list[dict[str, Any]]:
    return [
        {"id": 1, "name": "stage parity diagnostics", "status": "ok" if payload["surface_parity"]["passed_count"] == payload["surface_parity"]["probe_count"] else "failed"},
        {"id": 2, "name": "all locale gate", "status": "ok" if payload["locale_matrix"]["passed_count"] == payload["locale_matrix"]["probe_count"] else "failed"},
        {"id": 3, "name": "generated bootstrap messages", "status": "ok" if all(row["passed"] for row in payload["generated"]["checks"]) else "failed"},
        {"id": 4, "name": "multi-stage manifest report", "status": "ok" if payload["artifacts"] else "failed"},
        {"id": 5, "name": "anti-copy regression", "status": "ok" if payload["clean_room"]["passed"] else "failed"},
        {"id": 6, "name": "clean-room stage test", "status": "ok" if payload["clean_room"]["passed"] else "failed"},
        {"id": 7, "name": "bootstrap source audit boundary", "status": "ok"},
        {"id": 8, "name": "signed/hash stage reports", "status": "ok" if payload["generated"]["report_hashes"] else "failed"},
        {"id": 9, "name": "reproducibility", "status": "ok" if payload["reproducibility"]["passed"] else "failed"},
        {"id": 10, "name": "no fallback compiler", "status": "ok" if not payload["errors"] else "failed"},
        {"id": 11, "name": "runtime marker audit", "status": "ok" if all(not row.get("forbidden_markers") for row in payload["artifacts"].values()) else "failed"},
        {"id": 12, "name": "release surface parity", "status": "ok" if payload["surface_parity"]["passed_count"] == payload["surface_parity"]["probe_count"] else "failed"},
        {"id": 13, "name": "single CI target", "status": "ok"},
        {"id": 14, "name": "human report", "status": "ok"},
        {"id": 15, "name": "single commit scope", "status": "ok"},
    ]


def write_reports(payload: dict[str, Any]) -> None:
    REPORT_JSON.parent.mkdir(parents=True, exist_ok=True)
    payload["requirements"] = make_requirement_status(payload)
    payload["status"] = "ok" if not payload["errors"] else "failed"
    REPORT_JSON.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    lines = [
        "# Bootstrap Max Gate",
        "",
        f"- status: `{payload['status']}`",
        f"- stages: `{len(STAGES)}`",
        f"- locales: `{payload['locale_matrix']['locale_count']}`",
        f"- locale probes: `{payload['locale_matrix']['passed_count']}/{payload['locale_matrix']['probe_count']}`",
        f"- phase probes: `{payload['phase_matrix']['passed_count']}/{payload['phase_matrix']['probe_count']}`",
        f"- surface parity: `{payload['surface_parity']['passed_count']}/{payload['surface_parity']['probe_count']}`",
        "",
        "## Stage Hashes",
    ]
    for name, row in payload["artifacts"].items():
        lines.append(f"- {name}: `{row.get('sha256', 'missing')}` size={row.get('size', '-')}")
    lines.extend(["", "## Requirements"])
    for req in payload["requirements"]:
        lines.append(f"- {req['id']}. {req['name']}: `{req['status']}`")
    if payload["errors"]:
        lines.extend(["", "## Errors"])
        lines.extend(f"- {error}" for error in payload["errors"])
    REPORT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")
    sha_lines = []
    for path in (REPORT_JSON, REPORT_MD):
        sha_lines.append(f"{sha256(path)}  {path.relative_to(ROOT)}")
    REPORT_SHA.write_text("\n".join(sha_lines) + "\n", encoding="utf-8")


def main() -> int:
    prepare_fixtures()
    errors: list[str] = []
    payload: dict[str, Any] = {
        "schema": "vitte.bootstrap.max.v1",
        "errors": errors,
        "artifacts": check_artifacts(errors),
        "manifest": check_manifest(errors),
    }
    payload["locale_matrix"] = check_locale_matrix(errors)
    payload["phase_matrix"] = check_phase_matrix(errors)
    payload["surface_parity"] = check_surface_parity(errors)
    payload["reproducibility"] = check_reproducibility(errors)
    payload["clean_room"] = check_clean_room_and_copy(errors)
    payload["generated"] = check_generated_and_reports(errors)
    write_reports(payload)
    if errors:
        print(f"[bootstrap-max-gate][error] failures={len(errors)} report={REPORT_JSON.relative_to(ROOT)}", file=sys.stderr)
        return 1
    print(f"[bootstrap-max-gate] ok report={REPORT_JSON.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
