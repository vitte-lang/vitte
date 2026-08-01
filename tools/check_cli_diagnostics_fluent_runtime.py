#!/usr/bin/env python3
from __future__ import annotations

import json
import os
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
EXPECTED_RICH_TEXT_TERMS = (
    "= id:",
    "= category: typeck",
    "= severity: error",
    f"= fluent-key: {EXPECTED_CODE}",
    "= span: tests/negative/type_mismatch.vit:5:11-5:18",
    "= label:",
    f"= cause: {EXPECTED_CAUSE}",
    "= help: Compare the expected and found types in the diagnostic labels.",
    f"= fix-it: {EXPECTED_FIX}",
    "= corrected example: let count: int = 1",
)


def run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        args,
        cwd=ROOT,
        env={**os.environ, "VITTE_LANG": "en"},
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )


def fail(message: str) -> int:
    print(f"[cli-diagnostics-fluent-runtime][error] {message}", file=sys.stderr)
    return 1


def assert_french_text_surface(name: str, proc: subprocess.CompletedProcess[str]) -> str | None:
    output = proc.stdout + proc.stderr
    if proc.returncode == 0:
        return f"{name} diagnostic fixture unexpectedly passed"
    if EXPECTED_CODE not in output:
        return f"{name} output missing {EXPECTED_CODE}"
    if EXPECTED_FR_MESSAGE not in output:
        return f"{name} output does not carry the Fluent FR message"
    if EXPECTED_EN_MESSAGE in output:
        return f"{name} output still carries the default EN diagnostic message"
    if "[vitte][error]" in output or "E_CLI_IO: cannot read" in output:
        return f"{name} output contains a legacy raw diagnostic"
    for term in (EXPECTED_CAUSE, EXPECTED_FIX, EXPECTED_EXAMPLE, *EXPECTED_RICH_TEXT_TERMS):
        if term not in output:
            return f"{name} text output missing rich diagnostic term: {term!r}"
    return None


def main() -> int:
    default_check = run([str(BIN), "check", str(FIXTURE)])
    default_output = default_check.stdout + default_check.stderr
    if default_check.returncode == 0:
        return fail("diagnostic fixture unexpectedly passed with default locale")
    if EXPECTED_CODE not in default_output:
        return fail(f"default output missing {EXPECTED_CODE}")
    if EXPECTED_EN_MESSAGE not in default_output:
        return fail("default output does not carry the Fluent EN message")
    for term in (EXPECTED_CAUSE, EXPECTED_FIX, EXPECTED_EXAMPLE, *EXPECTED_RICH_TEXT_TERMS):
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
    for term in (EXPECTED_CAUSE, EXPECTED_FIX, EXPECTED_EXAMPLE, *EXPECTED_RICH_TEXT_TERMS):
        if term not in output:
            return fail(f"fr text output missing rich diagnostic term: {term!r}")

    alias = run([str(BIN), "check", str(FIXTURE), "--lang=fr-FR"])
    alias_output = alias.stdout + alias.stderr
    if alias.returncode == 0:
        return fail("diagnostic fixture unexpectedly passed with fr-FR alias")
    if EXPECTED_FR_MESSAGE not in alias_output:
        return fail("fr-FR alias output does not carry the Fluent FR message")

    build_output = ROOT / "target" / "check" / f"fluent-runtime-build-{os.getpid()}"
    build_output.parent.mkdir(parents=True, exist_ok=True)
    command_surfaces = (
        ("check", [str(BIN), "check", str(FIXTURE), "--lang", "fr"]),
        ("build", [str(BIN), "build", str(FIXTURE), "-o", str(build_output), "--lang", "fr"]),
        ("run", [str(BIN), "run", str(FIXTURE), "--lang", "fr"]),
        ("test", [str(BIN), "test", str(FIXTURE), "--lang", "fr"]),
    )
    for name, command in command_surfaces:
        failure = assert_french_text_surface(name, run(command))
        if failure is not None:
            return fail(failure)
    if build_output.exists():
        return fail("build created an output artifact after localized diagnostic preflight failed")

    json_check = run([str(BIN), "check", "--diagnostics-json", str(FIXTURE), "--lang", "fr"])
    if json_check.returncode == 0:
        return fail("diagnostics-json fixture unexpectedly passed")
    if json_check.stderr:
        return fail("diagnostics-json wrote to stderr")
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
    for key in ("id", "category", "severity", "fluent_key", "message", "span", "cause", "help"):
        if primary.get(key) in (None, "", []):
            return fail(f"diagnostics-json missing {key}")

    lsp_check = run([str(BIN), "check", "--diagnostics-lsp", str(FIXTURE), "--lang", "fr"])
    if lsp_check.returncode == 0:
        return fail("diagnostics-lsp fixture unexpectedly passed")
    if lsp_check.stderr:
        return fail("diagnostics-lsp wrote to stderr")
    try:
        lsp_payload = json.loads(lsp_check.stdout)
    except json.JSONDecodeError as exc:
        return fail(f"diagnostics-lsp output is invalid JSON: {exc}")
    lsp_diagnostics = lsp_payload.get("params", {}).get("diagnostics", [])
    if lsp_payload.get("jsonrpc") != "2.0" or lsp_payload.get("method") != "textDocument/publishDiagnostics":
        return fail("diagnostics-lsp is not a publishDiagnostics JSON-RPC notification")
    if len(lsp_diagnostics) != 1:
        return fail("diagnostics-lsp must contain one published diagnostic")
    lsp_primary = lsp_diagnostics[0]
    if lsp_primary.get("code") != EXPECTED_CODE:
        return fail(f"diagnostics-lsp code mismatch: {lsp_primary.get('code')!r}")
    if lsp_primary.get("message") != EXPECTED_FR_MESSAGE:
        return fail(f"diagnostics-lsp message mismatch: {lsp_primary.get('message')!r}")
    for key in ("range", "severity", "codeDescription", "source", "relatedInformation", "data"):
        if lsp_primary.get(key) in (None, "", []):
            return fail(f"diagnostics-lsp missing {key}")
    data = lsp_primary.get("data", {})
    if data.get("cause") != EXPECTED_CAUSE or data.get("fix") != EXPECTED_FIX or data.get("example") != EXPECTED_EXAMPLE:
        return fail("diagnostics-lsp missing cause/fix/example data")
    for key in ("code", "id", "category", "severity", "fluent_key", "message", "span", "labels", "suggestions", "code_action_title"):
        if data.get(key) in (None, "", []):
            return fail(f"diagnostics-lsp data missing {key}")

    print("[cli-diagnostics-fluent-runtime] status=ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
