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
STAGE1 = ROOT / "target/stage1/vitte"
STAGE2 = ROOT / "target/stage2/vitte"
ENTRYPOINT = "src/vitte/compiler/main.vit"
NEGATIVE = "tests/negative/type_mismatch.vit"
BUILD_FIXTURE = "tests/check/main.vit"
OUT = ROOT / "target/selfhost-stage-compare"
REPORT_DIR = ROOT / "target/reports"
REPORT_JSON = REPORT_DIR / "selfhost_stage_compare_gate.json"
REPORT_MD = REPORT_DIR / "selfhost_stage_compare_gate.md"
FORBIDDEN_RUNTIME_MARKERS = ("[vitte][error]", "E_CLI_IO: cannot read")


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
        "stderr": proc.stderr.strip(),
        "stdout": proc.stdout.strip(),
    }


def normalized(value: str) -> str:
    replacements = {
        str(ROOT): "<ROOT>",
        "target/selfhost-stage-compare/stage1-built": "target/selfhost-stage-compare/<built>",
        "target/selfhost-stage-compare/stage2-built": "target/selfhost-stage-compare/<built>",
        "stage1-built": "<built>",
        "stage2-built": "<built>",
    }
    for old, new in replacements.items():
        value = value.replace(old, new)
    return value


def compare_command(
    name: str,
    args: list[str],
    failures: list[str],
    comparisons: list[dict[str, Any]],
    *,
    expect_equal: bool = True,
) -> tuple[dict[str, Any], dict[str, Any]]:
    left = run(STAGE1, args)
    right = run(STAGE2, args)
    left_norm = {
        "exit_code": left["exit_code"],
        "stdout": normalized(left["stdout"]),
        "stderr": normalized(left["stderr"]),
    }
    right_norm = {
        "exit_code": right["exit_code"],
        "stdout": normalized(right["stdout"]),
        "stderr": normalized(right["stderr"]),
    }
    equal = left_norm == right_norm
    if expect_equal and not equal:
        failures.append(f"{name} differs between stage1 and stage2")
    comparisons.append(
        {
            "name": name,
            "args": args,
            "equal": equal,
            "stage1": left_norm,
            "stage2": right_norm,
        }
    )
    return left, right


def validate_compiler_provenance(path: Path, failures: list[str], *, compiler_input: Path | None = None) -> None:
    strings = subprocess.run(
        ["strings", str(path)],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        check=False,
    ).stdout
    for marker in FORBIDDEN_RUNTIME_MARKERS:
        if marker in strings:
            failures.append(f"{path.relative_to(ROOT)} contains obsolete runtime marker: {marker}")
    symbols = subprocess.run(
        ["nm", str(path)],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        check=False,
    ).stdout
    if ("_command_build" in symbols and "_copy_file" in symbols) or "_vitte_stage0_clone_self" in symbols:
        if compiler_input is not None and compiler_input.is_file() and sha256_file(path) == sha256_file(compiler_input):
            failures.append(f"{path.relative_to(ROOT)} is a byte-for-byte compiler copy produced by the self-copy dispatcher")
        else:
            failures.append(f"{path.relative_to(ROOT)} still contains the self-copy dispatcher")
    if "run_cli_main_with_ice_boundary" not in symbols:
        failures.append(f"{path.relative_to(ROOT)} does not contain run_cli_main_with_ice_boundary")


def ensure_stage_binaries(failures: list[str], comparisons: list[dict[str, Any]]) -> None:
    if not STAGE1.is_file():
        failures.append("missing target/stage1/vitte")
        return
    if not os.access(STAGE1, os.X_OK):
        failures.append("target/stage1/vitte is not executable")
        return
    if b"vitte_stage0_clone_self" in STAGE1.read_bytes():
        failures.append("refusing to execute stage1 because it contains the retired self-copy implementation")
        return
    validate_compiler_provenance(STAGE1, failures)
    OUT.mkdir(parents=True, exist_ok=True)
    build = run(STAGE1, ["build", ENTRYPOINT, "-o", str(STAGE2.relative_to(ROOT))])
    comparisons.append(
        {
            "name": "rebuild-stage2",
            "args": build["args"],
            "equal": build["exit_code"] == 0,
            "stage1": {
                "exit_code": build["exit_code"],
                "stdout": normalized(build["stdout"]),
                "stderr": normalized(build["stderr"]),
            },
        }
    )
    if build["exit_code"] != 0:
        failures.append(f"stage1 could not rebuild stage2: {build['stderr'] or build['stdout']}")
    if not STAGE2.is_file():
        failures.append("missing target/stage2/vitte")
    elif not os.access(STAGE2, os.X_OK):
        failures.append("target/stage2/vitte is not executable")
    else:
        validate_compiler_provenance(STAGE2, failures, compiler_input=STAGE1)


def compare_reproducible_build_hashes(failures: list[str], comparisons: list[dict[str, Any]]) -> None:
    stage1_out = OUT / "stage1-built"
    stage2_out = OUT / "stage2-built"
    left, right = compare_command(
        "build-fixture",
        ["build", BUILD_FIXTURE, "-o", str(stage1_out.relative_to(ROOT))],
        failures,
        comparisons,
        expect_equal=False,
    )
    # Re-run the right command with its own output path; compare normalized diagnostics and output hash.
    right = run(STAGE2, ["build", BUILD_FIXTURE, "-o", str(stage2_out.relative_to(ROOT))])
    comparisons[-1]["stage2"] = {
        "exit_code": right["exit_code"],
        "stdout": normalized(right["stdout"]),
        "stderr": normalized(right["stderr"]),
    }
    comparisons[-1]["equal"] = comparisons[-1]["stage1"] == comparisons[-1]["stage2"]
    if comparisons[-1]["stage1"] != comparisons[-1]["stage2"]:
        failures.append("build-fixture diagnostics differ between stage1 and stage2")
    if left["exit_code"] != 0 or right["exit_code"] != 0:
        failures.append("build-fixture failed on stage1 or stage2")
        return
    if not stage1_out.is_file() or not stage2_out.is_file():
        failures.append("build-fixture did not produce both outputs")
        return
    stage1_hash = sha256_file(stage1_out)
    stage2_hash = sha256_file(stage2_out)
    same_hash = stage1_hash == stage2_hash
    hash_enforced = sys.platform != "darwin"
    if hash_enforced and not same_hash:
        failures.append("build-fixture output hash differs between stage1 and stage2")
    comparisons.append(
        {
            "name": "build-fixture-sha256",
            "equal": same_hash or not hash_enforced,
            "hash_enforced": hash_enforced,
            "reason": "" if same_hash or hash_enforced else "darwin native binaries include linker load-command entropy; behavior, diagnostics, MIR, and IR are compared instead",
            "stage1_sha256": stage1_hash,
            "stage2_sha256": stage2_hash,
        }
    )


def write_reports(status: str, failures: list[str], comparisons: list[dict[str, Any]]) -> None:
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    report = {
        "schema": "vitte.selfhost.stage.compare.gate.v1",
        "status": status,
        "stage1": "target/stage1/vitte",
        "stage2": "target/stage2/vitte",
        "entrypoint": ENTRYPOINT,
        "negative_fixture": NEGATIVE,
        "build_fixture": BUILD_FIXTURE,
        "comparison_count": len(comparisons),
        "comparisons": comparisons,
        "failures": failures,
    }
    REPORT_JSON.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    lines = [
        "# selfhost stage compare gate",
        "",
        f"- status: {status}",
        "- stage1: target/stage1/vitte",
        "- stage2: target/stage2/vitte",
        f"- comparisons: {len(comparisons)}",
    ]
    if failures:
        lines.append("")
        lines.append("## Failures")
        lines.extend(f"- {failure}" for failure in failures)
    REPORT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    failures: list[str] = []
    comparisons: list[dict[str, Any]] = []
    ensure_stage_binaries(failures, comparisons)
    if not failures:
        compare_command("version", ["--version"], failures, comparisons)
        compare_command("help", ["--help"], failures, comparisons)
        compare_command("check-entrypoint", ["check", ENTRYPOINT], failures, comparisons)
        compare_command("diagnostics-negative", ["check", NEGATIVE], failures, comparisons)
        compare_command("native-ir", ["dump-native-ir", "--src", ENTRYPOINT], failures, comparisons)
        compare_command("mir", ["dump-mir", ENTRYPOINT], failures, comparisons)
        compare_reproducible_build_hashes(failures, comparisons)
    status = "fail" if failures else "pass"
    write_reports(status, failures, comparisons)
    if failures:
        print("[selfhost-stage-compare-gate][error] stage comparison failed", file=sys.stderr)
        for failure in failures:
            print(f" - {failure}", file=sys.stderr)
        return 1
    print("[selfhost-stage-compare-gate] ok stage1=stage2")
    return 0


if __name__ == "__main__":
    sys.exit(main())
