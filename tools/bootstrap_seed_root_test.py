#!/usr/bin/env python3
"""Regression tests for the seed-only bootstrap trust contract."""

from __future__ import annotations

import copy
import json
import re
from pathlib import Path

import check_bootstrap_seed_root as contract

ROOT = Path(__file__).resolve().parents[1]
FORBIDDEN_NUMBERED_STAGE = re.compile(r"\bstage[ _.\-]?[1-4]\b", re.IGNORECASE)


def expect_rejected(config: dict[str, object], label: str) -> None:
    try:
        contract.validate_contract(config)
    except contract.ContractError:
        return
    raise AssertionError(f"invalid bootstrap contract accepted: {label}")


def active_bootstrap_files() -> list[Path]:
    files = [
        ROOT / "Makefile",
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
    trust_root = contract.validate_contract(config)
    assert trust_root["artifact"] == "toolchain/seed/vittec0.seed"

    with_stages = copy.deepcopy(config)
    bootstrap = with_stages["bootstrap"]
    assert isinstance(bootstrap, dict)
    bootstrap["stages"] = [{"stage": 1, "compiler": "bin/vitte"}]
    expect_rejected(with_stages, "numbered stages")

    with_fallback = copy.deepcopy(config)
    bootstrap = with_fallback["bootstrap"]
    assert isinstance(bootstrap, dict)
    root = bootstrap["trust_root"]
    assert isinstance(root, dict)
    root["compiler"] = "bin/vitte"
    expect_rejected(with_fallback, "alternate compiler trust root")

    violations = []
    for path in active_bootstrap_files():
        for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            if FORBIDDEN_NUMBERED_STAGE.search(line):
                violations.append(f"{path.relative_to(ROOT)}:{line_number}:{line.strip()}")
    if violations:
        raise AssertionError("active numbered bootstrap references:\n" + "\n".join(violations))

    print("[bootstrap-seed-root-test] OK trust_root=toolchain/seed/vittec0.seed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
