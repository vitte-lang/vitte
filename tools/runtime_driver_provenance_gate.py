#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "bin/vitte"
ENTRY = ROOT / "src/vitte/compiler/main.vit"
DRIVER = ROOT / "src/vitte/compiler/driver/compiler.vit"
ALTERNATIVE_DRIVER_PATHS = (
    ROOT / "src/vitte/compiler/driver/mod.vit",
    ROOT / "src/vitte/compiler/driver/cli.vit",
    ROOT / "src/vitte/compiler/driver/compile.vit",
)
REPORT = ROOT / "target/reports/runtime_driver_provenance.json"
FORBIDDEN_RUNTIME_MARKERS = (b"[vitte][error]", b"E_CLI_IO: cannot read")


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command: list[str]) -> dict[str, Any]:
    proc = subprocess.run(
        command,
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    return {
        "command": command,
        "exit_code": proc.returncode,
        "stdout": proc.stdout,
        "stderr": proc.stderr,
    }


def main() -> int:
    failures: list[str] = []
    evidence: dict[str, Any] = {}
    source_chain: dict[str, Any] = {}
    if not ENTRY.is_file() or not DRIVER.is_file():
        failures.append("canonical Vitte driver source chain is incomplete")
    else:
        entry_text = ENTRY.read_text(encoding="utf-8")
        driver_text = DRIVER.read_text(encoding="utf-8")
        source_chain = {
            "entry_imports_driver": "use vitte/compiler/driver/compiler.{ run_cli_main_with_ice_boundary }" in entry_text,
            "entry_calls_driver": "let code: int = run_cli_main_with_ice_boundary(args);" in entry_text,
            "driver_defines_run_cli_main": "proc run_cli_main(args: list[string]) -> int" in driver_text,
            "driver_defines_ice_boundary": "proc run_cli_main_with_ice_boundary(args: list[string]) -> int" in driver_text,
            "ice_boundary_parses_request": "let request: CompilerCliRequest = parse_cli_request(args);" in driver_text,
            "ice_boundary_runs_request": "give run_cli_request(request);" in driver_text,
        }
        for name, present in source_chain.items():
            if not present:
                failures.append(f"canonical source chain assertion failed: {name}")

        alternative_definitions: list[str] = []
        for path in ALTERNATIVE_DRIVER_PATHS:
            text = path.read_text(encoding="utf-8")
            if "proc run_cli_main(" in text or "proc run_cli_main_with_ice_boundary(" in text:
                alternative_definitions.append(path.relative_to(ROOT).as_posix())
        source_chain["alternative_runtime_definitions"] = alternative_definitions
        if alternative_definitions:
            failures.append("alternate driver modules redefine the process runtime entry")

    if not BIN.is_file():
        failures.append("bin/vitte is missing")
    else:
        binary_data = BIN.read_bytes()
        embedded_markers = [marker.decode("ascii") for marker in FORBIDDEN_RUNTIME_MARKERS if marker in binary_data]
        evidence["embedded_obsolete_diagnostic_markers"] = embedded_markers
        for marker in embedded_markers:
            failures.append(f"binary contains obsolete runtime diagnostic marker: {marker}")

        symbols = run(["nm", str(BIN)])
        symbol_text = symbols["stdout"] + symbols["stderr"]
        copy_symbols = [name for name in ("_command_build", "_copy_file") if name in symbol_text]
        run_cli_symbols = [line for line in symbol_text.splitlines() if "run_cli_main" in line]
        ice_boundary_symbols = [line for line in run_cli_symbols if "run_cli_main_with_ice_boundary" in line]
        evidence["copy_symbols"] = copy_symbols
        evidence["run_cli_main_symbols"] = run_cli_symbols
        evidence["run_cli_main_with_ice_boundary_symbols"] = ice_boundary_symbols
        evidence["binary_dispatch_symbols"] = [
            name for name in ("_main", "_command_check", "_command_build", "_command_test", "_command_package_like")
            if name in symbol_text
        ]
        if evidence["binary_dispatch_symbols"] and not run_cli_symbols:
            failures.append(
                "binary exposes its native command dispatcher but contains no run_cli_main runtime symbol"
            )
        if not ice_boundary_symbols:
            failures.append("binary contains no run_cli_main_with_ice_boundary runtime symbol")
        if copy_symbols:
            failures.append(
                "runtime build path contains self-copy implementation symbols: " + ", ".join(copy_symbols)
            )

        with tempfile.TemporaryDirectory(prefix="vitte-provenance-") as directory:
            output = Path(directory) / "vitte"
            build = run([str(BIN), "build", str(ENTRY.relative_to(ROOT)), "-o", str(output)])
            evidence["build"] = build
            if build["exit_code"] != 0:
                failures.append("canonical compiler entry build failed during provenance probe")
            elif not output.is_file():
                failures.append("canonical compiler entry build did not materialize an artifact")
            else:
                input_hash = sha256(BIN)
                output_hash = sha256(output)
                evidence["input_sha256"] = input_hash
                evidence["output_sha256"] = output_hash
                evidence["byte_identical"] = input_hash == output_hash
                if input_hash == output_hash and copy_symbols:
                    failures.append(
                        "building src/vitte/compiler/main.vit reproduces bin/vitte byte-for-byte through the self-copy path"
                    )

            probe_root = Path(directory) / "src/vitte/compiler"
            shutil.copytree(ROOT / "src/vitte/compiler", probe_root)
            probe_entry = probe_root / "main.vit"
            probe_driver = probe_root / "driver/compiler.vit"
            probe_output = Path(directory) / "compiler-provenance"
            baseline_build = run([str(BIN), "build", str(probe_entry), "-o", str(probe_output)])
            evidence["source_sensitivity_baseline_build"] = baseline_build
            baseline_hash = sha256(probe_output) if baseline_build["exit_code"] == 0 and probe_output.is_file() else ""
            driver_text = probe_driver.read_text(encoding="utf-8")
            perturbed_text = driver_text.replace(
                'const VERSION_TEXT: string = "vittec vitte-compiler 0.1.0"',
                'const VERSION_TEXT: string = "vittec vitte-compiler provenance-probe"',
                1,
            )
            if perturbed_text == driver_text:
                failures.append("compiler source sensitivity probe could not locate VERSION_TEXT")
            else:
                probe_driver.write_text(perturbed_text, encoding="utf-8")
                probe_output.unlink(missing_ok=True)
                perturbed_build = run([str(BIN), "build", str(probe_entry), "-o", str(probe_output)])
                evidence["source_sensitivity_perturbed_build"] = perturbed_build
                perturbed_hash = sha256(probe_output) if perturbed_build["exit_code"] == 0 and probe_output.is_file() else ""
                evidence["source_sensitivity"] = {
                    "baseline_sha256": baseline_hash,
                    "perturbed_sha256": perturbed_hash,
                    "output_changed": bool(baseline_hash and perturbed_hash and baseline_hash != perturbed_hash),
                }
                if not baseline_hash:
                    failures.append("compiler source sensitivity baseline did not materialize an artifact")
                if not perturbed_hash:
                    failures.append("modified compiler source did not materialize an artifact")
                if baseline_hash and perturbed_hash and baseline_hash == perturbed_hash:
                    failures.append("modifying compiler.vit does not change the rebuilt artifact")

    report = {
        "schema": "vitte.runtime-driver-provenance.v1",
        "status": "failed" if failures else "ok",
        "binary": "bin/vitte",
        "entrypoint": "src/vitte/compiler/main.vit",
        "required_runtime_entry": "run_cli_main_with_ice_boundary",
        "source_chain": source_chain,
        "evidence": evidence,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    if failures:
        for failure in failures:
            print(f"[runtime-driver-provenance][error] {failure}", file=sys.stderr)
        print(f"[runtime-driver-provenance] status=failed report={REPORT.relative_to(ROOT)}", file=sys.stderr)
        return 1
    print(f"[runtime-driver-provenance] status=ok report={REPORT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
