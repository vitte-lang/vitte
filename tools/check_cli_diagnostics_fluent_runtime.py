#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "bin" / "vitte"
FIXTURE = ROOT / "examples" / "bad_build_example.vit"
EXPECTED_CODE = "TYPECK_E_ASSIGN_MISMATCH"
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
    check = run([str(BIN), "check", "--diagnostics-json", str(FIXTURE), "--lang", "fr"])
    if check.returncode == 0:
        return fail("diagnostic fixture unexpectedly passed")
    try:
        payload = json.loads(check.stdout)
    except json.JSONDecodeError as exc:
        return fail(f"diagnostics JSON is invalid: {exc}")

    diagnostics = payload.get("primary_report", {}).get("diagnostics", [])
    if not diagnostics:
        return fail("diagnostics JSON does not contain a primary diagnostic")
    primary = diagnostics[0]
    if primary.get("code") != EXPECTED_CODE:
        return fail(f"expected {EXPECTED_CODE}, got {primary.get('code')!r}")
    if primary.get("message") != EXPECTED_FR_MESSAGE:
        return fail(f"expected Fluent FR message {EXPECTED_FR_MESSAGE!r}, got {primary.get('message')!r}")
    if EXPECTED_FR_MESSAGE not in payload.get("text_output", ""):
        return fail("text_output does not carry the Fluent FR message")

    build = run([str(BIN), "build", str(FIXTURE), "-o", str(ROOT / "target" / "fluent-runtime-probe"), "--lang", "fr"])
    if build.returncode == 0:
        return fail("build diagnostic fixture unexpectedly built")
    combined = build.stdout + build.stderr
    if EXPECTED_CODE not in combined:
        return fail(f"build output missing {EXPECTED_CODE}")
    if EXPECTED_FR_MESSAGE not in combined:
        return fail("build output does not carry the Fluent FR message")

    print("[cli-diagnostics-fluent-runtime] status=ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
