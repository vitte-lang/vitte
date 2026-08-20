#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
STAGES = {
    "stage1": ROOT / "target/stage1/vitte",
    "stage2": ROOT / "target/stage2/vitte",
    "release": ROOT / "target/release/vitte",
}
ENTRYPOINT = "src/vitte/compiler/main.vit"
NEGATIVE = "tests/negative/type_mismatch.vit"
NATIVE_FIXTURE = "tests/check/main.vit"
OUT = ROOT / "target/selfhost-stage0"
REPORT_DIR = ROOT / "target/reports"
REPORT_JSON = REPORT_DIR / "selfhost_stage0_gate.json"
REPORT_MD = REPORT_DIR / "selfhost_stage0_gate.md"
FORBIDDEN_RUNTIME_MARKERS = (b"[vitte][error]", b"E_CLI_IO: cannot read")


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def env_for(stage: Path) -> dict[str, str]:
    env = dict(os.environ)
    env["VITTE_ROOT"] = str(ROOT)
    env["VITTE_COMPILER"] = str(stage)
    env["VITTE_PACKAGE_OFFLINE"] = "1"
    return env


def run(stage: Path, args: list[str]) -> dict[str, Any]:
    proc = subprocess.run(
        [str(stage), *args],
        cwd=ROOT,
        env=env_for(stage),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    return {
        "args": args,
        "exit_code": proc.returncode,
        "stdout": proc.stdout.strip(),
        "stderr": proc.stderr.strip(),
    }


def normalize(text: str) -> str:
    value = text.replace(str(ROOT), "<ROOT>")
    for stage in STAGES:
        value = value.replace(f"target/selfhost-stage0/{stage}", "target/selfhost-stage0/<stage>")
        value = value.replace(f"{stage}-compiler", "<stage>-compiler")
        value = value.replace(f"{stage}-native", "<stage>-native")
    return value


def normalized_result(result: dict[str, Any]) -> dict[str, Any]:
    return {
        "exit_code": result["exit_code"],
        "stdout": normalize(result["stdout"]),
        "stderr": normalize(result["stderr"]),
    }


def compare_results(name: str, results: dict[str, dict[str, Any]], failures: list[str], comparisons: list[dict[str, Any]]) -> None:
    normalized = {stage: normalized_result(result) for stage, result in results.items()}
    first_stage = next(iter(normalized))
    first_value = normalized[first_stage]
    equal = all(value == first_value for value in normalized.values())
    if not equal:
        failures.append(f"{name} differs across stage1/stage2/release")
    comparisons.append({"name": name, "equal": equal, "results": normalized})


def compare_hashes(name: str, files: dict[str, Path], failures: list[str], comparisons: list[dict[str, Any]], *, enforce: bool) -> None:
    hashes = {stage: sha256_file(path) for stage, path in files.items() if path.is_file()}
    missing = [stage for stage, path in files.items() if not path.is_file()]
    if missing:
        failures.append(f"{name} missing outputs for: {', '.join(missing)}")
    first_hash = next(iter(hashes.values()), "")
    equal = bool(hashes) and all(value == first_hash for value in hashes.values()) and not missing
    if enforce and not equal:
        failures.append(f"{name} hash differs without declared reason")
    reason = ""
    if not enforce and not equal:
        reason = "native executable hash is not enforced on Darwin because Mach-O link output carries platform load-command entropy; diagnostics, IR, MIR, and execution are compared"
    comparisons.append(
        {
            "name": name,
            "equal": equal or not enforce,
            "hash_enforced": enforce,
            "reason": reason,
            "sha256": hashes,
            "missing": missing,
        }
    )


def ensure_stages(failures: list[str]) -> None:
    for name, path in STAGES.items():
        if not path.is_file():
            failures.append(f"missing {name}: {path.relative_to(ROOT)}")
        elif not os.access(path, os.X_OK):
            failures.append(f"{name} is not executable: {path.relative_to(ROOT)}")
        else:
            validate_compiler_provenance(name, path, failures)


def validate_compiler_provenance(
    name: str,
    path: Path,
    failures: list[str],
    *,
    compiler_input: Path | None = None,
) -> None:
    data = path.read_bytes()
    for marker in FORBIDDEN_RUNTIME_MARKERS:
        if marker in data:
            failures.append(f"{name} contains obsolete runtime diagnostic marker: {marker.decode('ascii')}")

    symbols_result = subprocess.run(
        ["nm", str(path)],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if symbols_result.returncode != 0:
        failures.append(f"{name} runtime symbols cannot be inspected")
        return
    symbols = symbols_result.stdout
    has_copy_dispatcher = (
        ("_command_build" in symbols and "_copy_file" in symbols)
        or "_vitte_stage0_clone_self" in symbols
    )
    if has_copy_dispatcher:
        if compiler_input is not None and compiler_input.is_file() and data == compiler_input.read_bytes():
            failures.append(f"{name} is a byte-for-byte compiler copy produced by the self-copy dispatcher")
        else:
            failures.append(f"{name} contains the self-copy compiler dispatcher")
    if "run_cli_main_with_ice_boundary" not in symbols:
        failures.append(f"{name} does not contain run_cli_main_with_ice_boundary")


def build_compilers(failures: list[str], comparisons: list[dict[str, Any]]) -> dict[str, Path]:
    outputs: dict[str, Path] = {}
    results: dict[str, dict[str, Any]] = {}
    for name, stage in STAGES.items():
        out = OUT / name / "vitte"
        out.parent.mkdir(parents=True, exist_ok=True)
        outputs[name] = out
        if not stage.is_file():
            failures.append(f"missing {name}: {stage.relative_to(ROOT)}")
            continue
        if b"vitte_stage0_clone_self" in stage.read_bytes():
            failures.append(f"refusing to execute {name} because it contains the retired self-copy implementation")
            continue
        result = run(stage, ["build", ENTRYPOINT, "-o", str(out.relative_to(ROOT))])
        results[name] = result
        if result["exit_code"] != 0:
            failures.append(f"{name} cannot build {ENTRYPOINT}: {result['stderr'] or result['stdout']}")
        elif out.is_file():
            validate_compiler_provenance(f"{name} compiler output", out, failures, compiler_input=stage)
    compare_results("build-compiler-diagnostics", results, failures, comparisons)
    return outputs


def compare_commands(failures: list[str], comparisons: list[dict[str, Any]]) -> None:
    command_matrix = {
        "version": ["--version"],
        "help": ["--help"],
        "diagnostics-entrypoint": ["check", ENTRYPOINT],
        "diagnostics-negative": ["check", NEGATIVE],
        "ir-entrypoint": ["dump-native-ir", "--src", ENTRYPOINT],
        "mir-entrypoint": ["dump-mir", ENTRYPOINT],
    }
    for name, args in command_matrix.items():
        results = {stage_name: run(stage, args) for stage_name, stage in STAGES.items()}
        compare_results(name, results, failures, comparisons)


def build_native_fixture(failures: list[str], comparisons: list[dict[str, Any]]) -> dict[str, Path]:
    outputs: dict[str, Path] = {}
    results: dict[str, dict[str, Any]] = {}
    for name, stage in STAGES.items():
        out = OUT / name / "native-fixture"
        out.parent.mkdir(parents=True, exist_ok=True)
        outputs[name] = out
        result = run(stage, ["build", NATIVE_FIXTURE, "-o", str(out.relative_to(ROOT))])
        results[name] = result
        if result["exit_code"] != 0:
            failures.append(f"{name} cannot build native fixture: {result['stderr'] or result['stdout']}")
    compare_results("build-native-fixture-diagnostics", results, failures, comparisons)
    return outputs


def write_reports(status: str, failures: list[str], comparisons: list[dict[str, Any]]) -> None:
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    report = {
        "schema": "vitte.selfhost.stage0.gate.v1",
        "status": status,
        "stages": {name: path.relative_to(ROOT).as_posix() for name, path in STAGES.items()},
        "entrypoint": ENTRYPOINT,
        "negative_fixture": NEGATIVE,
        "native_fixture": NATIVE_FIXTURE,
        "comparison_count": len(comparisons),
        "comparisons": comparisons,
        "failures": failures,
    }
    REPORT_JSON.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    lines = [
        "# selfhost stage0 gate",
        "",
        f"- status: {status}",
        f"- comparisons: {len(comparisons)}",
        f"- hash policy: {'strict' if sys.platform != 'darwin' else 'darwin-native-hash-declared'}",
    ]
    if failures:
        lines.append("")
        lines.append("## Failures")
        lines.extend(f"- {failure}" for failure in failures)
    REPORT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    failures: list[str] = []
    comparisons: list[dict[str, Any]] = []
    OUT.mkdir(parents=True, exist_ok=True)
    ensure_stages(failures)
    if not failures:
        compiler_outputs = build_compilers(failures, comparisons)
        compare_commands(failures, comparisons)
        native_outputs = build_native_fixture(failures, comparisons)
        compare_hashes("compiler-output-sha256", compiler_outputs, failures, comparisons, enforce=True)
        compare_hashes("native-fixture-sha256", native_outputs, failures, comparisons, enforce=sys.platform != "darwin")
    status = "fail" if failures else "pass"
    write_reports(status, failures, comparisons)
    if failures:
        print("[selfhost-stage0-gate][error] selfhost stage0 gate failed", file=sys.stderr)
        for failure in failures:
            print(f" - {failure}", file=sys.stderr)
        return 1
    print("[selfhost-stage0-gate] ok stages=stage1,stage2,release")
    return 0


if __name__ == "__main__":
    sys.exit(main())
