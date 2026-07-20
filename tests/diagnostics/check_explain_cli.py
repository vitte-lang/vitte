#!/usr/bin/env python3
from __future__ import annotations

import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
BIN = ROOT / "bin" / "vitte"


def run_explain(*args: str) -> str:
    proc = subprocess.run(
        [str(BIN), "explain", *args],
        cwd=str(ROOT),
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    if proc.returncode != 0:
        raise SystemExit(f"[explain-cli][error] {' '.join(args)} exited {proc.returncode}\n{proc.stdout}")
    return proc.stdout


def assert_contains(name: str, output: str, needles: tuple[str, ...]) -> None:
    for needle in needles:
        if needle not in output:
            raise SystemExit(f"[explain-cli][error] {name}: missing {needle!r}\n{output}")


def check_command() -> None:
    proc = subprocess.run(
        [str(BIN), "check", "tests/check/main.vit"],
        cwd=str(ROOT),
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    if proc.returncode != 0:
        raise SystemExit(f"[explain-cli][error] vitte check failed\n{proc.stdout}")
    if "check succeeded" not in proc.stdout:
        raise SystemExit(f"[explain-cli][error] vitte check did not report success\n{proc.stdout}")


def main() -> int:
    if not BIN.exists():
        raise SystemExit(f"[explain-cli][error] missing binary: {BIN}")

    assign = run_explain("TYPECK_E_ASSIGN_MISMATCH", "--lang", "en")
    assert_contains(
        "TYPECK_E_ASSIGN_MISMATCH",
        assign,
        (
            "error code: TYPECK_E_ASSIGN_MISMATCH",
            "lang: en",
            "phase: typeck",
            "message: assignment type mismatch",
            "cause:",
            "typical cases:",
            "correction:",
            "common pitfalls:",
            "fix:",
            "invalid example:",
            "valid example:",
        ),
    )

    alias = run_explain("E0001", "--lang", "fr-FR")
    assert_contains("fr-FR alias", alias, ("error code: E0001", "lang: fr", "phase: parser"))

    unknown = run_explain("NO_SUCH_CODE", "--lang=en")
    assert_contains(
        "unknown code fallback",
        unknown,
        (
            "error code: NO_SUCH_CODE",
            "phase: diagnostics",
            "full catalog",
            "typical cases:",
            "common pitfalls:",
            "vitte explain NO_SUCH_CODE --lang en",
        ),
    )

    check_command()

    print("[explain-cli] checked vitte explain and vitte check")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
