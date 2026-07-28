#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "bin" / "vitte"
FIXTURE = ROOT / "tests" / "negative" / "type_mismatch.vit"
EXPECTED_CODE = "TYPECK_E_ASSIGN_MISMATCH"
EXPECTED_EN_MESSAGE = "assignment type mismatch"
EXPECTED_FR_MESSAGE = "affectation type incompatibilite"
EXPECTED_CAUSE = "The inferred type does not satisfy the type required at this location."
EXPECTED_FIX = "assign a value of the declared binding type, or change the binding annotation at its declaration"
EXPECTED_EXAMPLE = "let count: int = 1"


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
    for term in (EXPECTED_CAUSE, EXPECTED_FIX, EXPECTED_EXAMPLE):
        if term not in default_output:
            return fail(f"default text output missing rich diagnostic term: {term!r}")

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
    for term in (EXPECTED_CAUSE, EXPECTED_FIX, EXPECTED_EXAMPLE):
        if term not in output:
            return fail(f"fr text output missing rich diagnostic term: {term!r}")

    alias = run([str(BIN), "check", str(FIXTURE), "--lang=fr-FR"])
    alias_output = alias.stdout + alias.stderr
    if alias.returncode == 0:
        return fail("diagnostic fixture unexpectedly passed with fr-FR alias")
    if EXPECTED_FR_MESSAGE not in alias_output:
        return fail("fr-FR alias output does not carry the Fluent FR message")

    json_check = run([str(BIN), "check", "--diagnostics-json", str(FIXTURE), "--lang", "fr"])
    if json_check.returncode == 0:
        return fail("diagnostics-json fixture unexpectedly passed")
    try:
        payload = json.loads(json_check.stdout)
    except json.JSONDecodeError as exc:
        return fail(f"diagnostics-json output is invalid JSON: {exc}")
    diagnostics = payload.get("primary_report", {}).get("diagnostics", [])
    if len(diagnostics) != 1:
        return fail("diagnostics-json must contain one runtime diagnostic")
    primary = diagnostics[0]
    if primary.get("code") != EXPECTED_CODE:
        return fail(f"diagnostics-json code mismatch: {primary.get('code')!r}")
    if primary.get("message") != EXPECTED_FR_MESSAGE:
        return fail(f"diagnostics-json message mismatch: {primary.get('message')!r}")
    if primary.get("cause") != EXPECTED_CAUSE:
        return fail("diagnostics-json missing stable cause")
    if primary.get("fix") != EXPECTED_FIX:
        return fail("diagnostics-json missing stable fix")
    if primary.get("example") != EXPECTED_EXAMPLE:
        return fail("diagnostics-json missing stable example")
    if not primary.get("labels") or not primary.get("suggestions"):
        return fail("diagnostics-json missing labels or suggestions")

    lsp_check = run([str(BIN), "check", "--diagnostics-lsp", str(FIXTURE), "--lang", "fr"])
    if lsp_check.returncode == 0:
        return fail("diagnostics-lsp fixture unexpectedly passed")
    try:
        lsp_payload = json.loads(lsp_check.stdout)
    except json.JSONDecodeError as exc:
        return fail(f"diagnostics-lsp output is invalid JSON: {exc}")
    lsp_diagnostics = lsp_payload.get("params", {}).get("diagnostics", [])
    if len(lsp_diagnostics) != 1:
        return fail("diagnostics-lsp must contain one published diagnostic")
    lsp_primary = lsp_diagnostics[0]
    if lsp_primary.get("code") != EXPECTED_CODE:
        return fail(f"diagnostics-lsp code mismatch: {lsp_primary.get('code')!r}")
    if lsp_primary.get("message") != EXPECTED_FR_MESSAGE:
        return fail(f"diagnostics-lsp message mismatch: {lsp_primary.get('message')!r}")
    data = lsp_primary.get("data", {})
    if data.get("cause") != EXPECTED_CAUSE or data.get("fix") != EXPECTED_FIX or data.get("example") != EXPECTED_EXAMPLE:
        return fail("diagnostics-lsp missing cause/fix/example data")

    print("[cli-diagnostics-fluent-runtime] status=ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
