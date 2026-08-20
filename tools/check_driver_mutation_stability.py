#!/usr/bin/env python3
"""Exercise driver failure paths so CLI regressions cannot pass silently."""

from __future__ import annotations

import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BOOTSTRAP = ROOT / "target" / "bootstrap-c17" / "vitte-bootstrap"
FIXTURE = ROOT / "tests" / "pipeline" / "hello_world.vit"
REPORT = ROOT / "target" / "reports" / "driver_mutation_stability.json"


def invoke(arguments: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(BOOTSTRAP), *arguments],
        cwd=ROOT,
        env=os.environ.copy(),
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )


def main() -> int:
    failures: list[str] = []
    checks: dict[str, bool] = {}

    valid = invoke(["check", str(FIXTURE)])
    checks["valid_driver_path"] = valid.returncode == 0
    if valid.returncode != 0:
        failures.append("valid driver invocation failed")

    missing = invoke(["check", str(ROOT / "tests" / "missing-driver-input.vit")])
    checks["missing_input_rejected"] = missing.returncode != 0
    checks["missing_input_diagnostic"] = "cannot open" in (missing.stdout + missing.stderr)
    if not checks["missing_input_rejected"]:
        failures.append("missing driver input was accepted")
    if not checks["missing_input_diagnostic"]:
        failures.append("missing driver input did not produce a diagnostic")

    unknown = invoke(["unknown-command", str(FIXTURE)])
    checks["unknown_command_rejected"] = unknown.returncode != 0
    checks["unknown_command_diagnostic"] = "unknown command" in (unknown.stdout + unknown.stderr)
    if not checks["unknown_command_rejected"]:
        failures.append("unknown driver command was accepted")
    if not checks["unknown_command_diagnostic"]:
        failures.append("unknown driver command did not produce a diagnostic")

    with tempfile.TemporaryDirectory(prefix="vitte-lexer-parser-mutation-") as directory:
        mutated = Path(directory) / "invalid-token.vit"
        mutated.write_text("proc main() -> int { give @; }\n", encoding="utf-8")
        syntax = invoke(["check", str(mutated)])
        syntax_output = syntax.stdout + syntax.stderr
        checks["lexer_mutation_rejected"] = syntax.returncode != 0
        checks["lexer_mutation_diagnostic"] = "VITTE_LEXER_E_TOKEN" in syntax_output
        checks["parser_mutation_diagnostic"] = "VITTE_PARSER_E_EXPR" in syntax_output
        if not checks["lexer_mutation_rejected"]:
            failures.append("illegal lexer mutation was accepted")
        if not checks["lexer_mutation_diagnostic"]:
            failures.append("illegal lexer mutation did not produce a lexer diagnostic")
        if not checks["parser_mutation_diagnostic"]:
            failures.append("illegal lexer mutation did not produce parser recovery diagnostics")

    with tempfile.TemporaryDirectory(prefix="vitte-backend-mutation-") as directory:
        workspace = Path(directory)
        build_env = os.environ.copy()
        build_env["VITTE_C17_GENERIC_COMPILER"] = "1"
        for label, expected in (("baseline", 0), ("mutated", 1)):
            source = workspace / f"{label}.vit"
            output = workspace / label
            source.write_text(f"proc main() -> int {{ give {expected}; }}\n", encoding="utf-8")
            build = subprocess.run(
                [str(BOOTSTRAP), "build", str(source), "-o", str(output)],
                cwd=ROOT,
                env=build_env,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                check=False,
            )
            execution = subprocess.run(
                [str(output)],
                cwd=ROOT,
                env=build_env,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                check=False,
            ) if output.is_file() else None
            checks[f"backend_{label}_build"] = build.returncode == 0 and output.is_file()
            checks[f"backend_{label}_execution"] = execution is not None and execution.returncode == expected
            if not checks[f"backend_{label}_build"]:
                failures.append(f"backend {label} mutation build failed")
            if not checks[f"backend_{label}_execution"]:
                actual = execution.returncode if execution is not None else "missing"
                failures.append(f"backend {label} mutation returned {actual}, expected {expected}")

    payload = {
        "schema": "vitte.compiler.driver_mutation_stability",
        "schema_version": "1.0.0",
        "status": "fail" if failures else "pass",
        "checks": checks,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"[driver-mutation] status={payload['status']} checks={sum(checks.values())}/{len(checks)}")
    for failure in failures:
        print(f"[driver-mutation][error] {failure}", file=sys.stderr)
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
