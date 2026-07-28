#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "bin" / "vitte"
FIXTURE = ROOT / "tests" / "negative" / "type_mismatch.vit"
EXPECTED_CODE = "TYPECK_E_ASSIGN_MISMATCH"
EXPECTED_EN_MESSAGE = "assignment type mismatch"
EXPECTED_FR_MESSAGE = "affectation type incompatibilite"


def run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        args,
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )


def fail(message: str) -> int:
    print(f"[cli-diagnostics-fluent-runtime][error] {message}", file=sys.stderr)
    return 1


def main() -> int:
    default_check = run([str(BIN), "check", str(FIXTURE)])
    default_output = default_check.stdout + default_check.stderr
    if default_check.returncode == 0:
        return fail("diagnostic fixture unexpectedly passed with default locale")
    if EXPECTED_CODE not in default_output:
        return fail(f"default output missing {EXPECTED_CODE}")
    if EXPECTED_EN_MESSAGE not in default_output:
        return fail("default output does not carry the Fluent EN message")

    check = run([str(BIN), "check", str(FIXTURE), "--lang", "fr"])
    output = check.stdout + check.stderr
    if check.returncode == 0:
        return fail("diagnostic fixture unexpectedly passed with fr locale")
    if EXPECTED_CODE not in output:
        return fail(f"fr output missing {EXPECTED_CODE}")
    if EXPECTED_FR_MESSAGE not in output:
        return fail("fr output does not carry the Fluent FR message")
    if EXPECTED_EN_MESSAGE in output:
        return fail("fr output still carries the default EN diagnostic message")

    alias = run([str(BIN), "check", str(FIXTURE), "--lang=fr-FR"])
    alias_output = alias.stdout + alias.stderr
    if alias.returncode == 0:
        return fail("diagnostic fixture unexpectedly passed with fr-FR alias")
    if EXPECTED_FR_MESSAGE not in alias_output:
        return fail("fr-FR alias output does not carry the Fluent FR message")

    print("[cli-diagnostics-fluent-runtime] status=ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
