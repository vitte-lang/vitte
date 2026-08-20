#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MODULE_PATH = ROOT / "tools/check_compiler_entrypoint.py"
spec = importlib.util.spec_from_file_location("check_compiler_entrypoint", MODULE_PATH)
assert spec is not None and spec.loader is not None
entrypoint_gate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(entrypoint_gate)


VALID = """
space vitte/compiler/main
use vitte/compiler/driver/compiler.{ run_cli_main_with_ice_boundary }
proc main(args: list[string]) -> int {
    let code: int = run_cli_main_with_ice_boundary(args);
    give code;
}
"""


def assert_valid(text: str) -> None:
    evidence, errors = entrypoint_gate.validate_runtime_entry_text(text)
    if errors:
        raise AssertionError(f"expected valid runtime entry, got {errors}; evidence={evidence}")


def assert_invalid(text: str, expected: str) -> None:
    _evidence, errors = entrypoint_gate.validate_runtime_entry_text(text)
    if not any(expected in error for error in errors):
        raise AssertionError(f"expected error containing {expected!r}, got {errors}")


def main() -> int:
    assert_valid(VALID)
    assert_invalid(
        VALID.replace(
            "    let code: int = run_cli_main_with_ice_boundary(args);",
            "    let code: int = 0;",
        )
        + "\nproc decoy(args: list[string]) -> int { give run_cli_main_with_ice_boundary(args); }\n",
        "exactly once",
    )
    assert_invalid(
        VALID.replace("use vitte/compiler/driver/compiler.{ run_cli_main_with_ice_boundary }\n", ""),
        "must import",
    )
    assert_invalid(
        VALID.replace("    give code;", "    give 0;"),
        "must return",
    )
    assert_valid(
        VALID.replace(
            "space vitte/compiler/main",
            '<<< run_cli_main_with_ice_boundary(args) >>>\nspace vitte/compiler/main\nconst TEXT: string = "run_cli_main_with_ice_boundary(args)"',
        )
    )
    print("[compiler-entrypoint-contract-test] ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
