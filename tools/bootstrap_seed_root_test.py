#!/usr/bin/env python3
"""Regression tests for the TOML bootstrap compiler and stdlib contract."""

from __future__ import annotations

import copy
import json
import re
from pathlib import Path

import check_bootstrap_seed_root as contract

ROOT = Path(__file__).resolve().parents[1]
FORBIDDEN_NUMBERED_STAGE = re.compile(r"\bstage[ _.\-]?[1-4]\b", re.IGNORECASE)


def allowed_numbered_stage_reference(path: Path, line: str) -> bool:
    relative = path.relative_to(ROOT).as_posix()
    if relative != "tools/bootstrap_config.py":
        return False
    stage_one = "compiler.stage" + "1"
    stage_two = "compiler.stage" + "2"
    tuple_fields = '"stage' + '1", "stage' + '2"'
    return stage_one in line or stage_two in line or tuple_fields in line


def expect_rejected(config: dict[str, object], label: str) -> None:
    try:
        contract.validate_contract(config)
    except contract.ContractError:
        return
    raise AssertionError(f"invalid bootstrap contract accepted: {label}")


def active_bootstrap_files() -> list[Path]:
    files = [
        ROOT / ".github/workflows/bootstrap.yml",
        ROOT / "toolchain/bootstrap.sh",
    ]
    for pattern in (
        "scripts/seed/*",
        "toolchain/scripts/bootstrap/*",
        "tools/bootstrap_*",
        "tools/check_bootstrap_*",
        "tools/check_seed_*",
    ):
        files.extend(path for path in ROOT.glob(pattern) if path.is_file())
    return sorted(set(files))


def main() -> int:
    config = contract.load_config(contract.DEFAULT_CONFIG)
    bootstrap_contract = contract.validate_contract(config)
    assert bootstrap_contract["artifact"] == "toolchain/seed/vittec0.seed"
    assert bootstrap_contract["compiler_entry"] == "src/vitte/compiler/main.vit"
    assert bootstrap_contract["stdlib"]["root"] == "src/vitte/stdlib"

    with_stages = copy.deepcopy(config)
    bootstrap = with_stages["bootstrap"]
    assert isinstance(bootstrap, dict)
    bootstrap["stages"] = [{"stage": 1, "compiler": "bin/vitte"}]
    expect_rejected(with_stages, "numbered stages")

    with_fallback = copy.deepcopy(config)
    compiler = with_fallback["compiler"]
    assert isinstance(compiler, dict)
    compiler["host_compiler"] = "bin/vitte"
    expect_rejected(with_fallback, "alternate compiler trust root")

    violations = []
    for path in active_bootstrap_files():
        for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            if FORBIDDEN_NUMBERED_STAGE.search(line) and not allowed_numbered_stage_reference(path, line):
                violations.append(f"{path.relative_to(ROOT)}:{line_number}:{line.strip()}")
    if violations:
        raise AssertionError("active numbered bootstrap references:\n" + "\n".join(violations))

    print("[bootstrap-compiler-stdlib-test] OK compiler=src/vitte/compiler/main.vit stdlib=src/vitte/stdlib")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
