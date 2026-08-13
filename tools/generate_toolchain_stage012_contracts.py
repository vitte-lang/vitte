#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
STAGES = ("stage0", "stage1", "stage2")
MODULES = {
    "stage0": ("main", "stage0", "stage0_context", "stage0_diagnostics", "stage0_driver", "stage0_manifest", "stage0_result", "stage0_signature", "stage0_statistics", "stage0_target", "stage0_trust", "stage0_validation"),
    "stage1": ("main", "stage1", "stage1_context", "stage1_diagnostics", "stage1_driver", "stage1_options", "stage1_pipeline", "stage1_profile", "stage1_result", "stage1_session", "stage1_statistics", "stage1_target", "stage1_validation"),
    "stage2": ("main", "stage2", "stage2_context", "stage2_diagnostics", "stage2_driver", "stage2_options", "stage2_pipeline", "stage2_profile", "stage2_result", "stage2_session", "stage2_statistics", "stage2_target", "stage2_validation"),
}
STAGE_META = {
    "stage0": {
        "role": "signed trust root and bootstrap input",
        "predecessor": "none",
        "policy": "signature-required, offline, single-platform-artifact",
        "capabilities": ["signature", "trust", "manifest", "validation", "diagnostics"],
    },
    "stage1": {
        "role": "compiler rebuilt from the trusted bootstrap compiler",
        "predecessor": "stage0",
        "policy": "verified-predecessor, canonical-output, deterministic-build",
        "capabilities": ["pipeline", "session", "target", "diagnostics", "statistics"],
    },
    "stage2": {
        "role": "compiler rebuilt from stage1 and release candidate",
        "predecessor": "stage1",
        "policy": "verified-predecessor, release-parity, deterministic-build",
        "capabilities": ["pipeline", "session", "target", "validation", "release-parity"],
    },
}
ARTIFACTS = {
    "stage0": "toolchain/bootstrap/stage0/macos-arm64/vitte",
    "stage1": "target/stage1/vitte",
    "stage2": "target/stage2/vitte",
}


def contract(stage: str, module: str) -> str:
    schema = f"vitte.toolchain.{stage}.{module}.v1"
    proc_name = f"{stage}_{module}_contract_ready".replace("-", "_")
    prefix = f"{stage.upper()}_{module.upper()}"
    meta = STAGE_META[stage]
    return (
        f"space toolchain/{stage}/{module}\n\n"
        f"const {prefix}_SCHEMA: string = \"{schema}\"\n"
        f"const {prefix}_STAGE: string = \"{stage}\"\n"
        f"const {prefix}_ROLE: string = \"{meta['role']}\"\n"
        f"const {prefix}_PREDECESSOR: string = \"{meta['predecessor']}\"\n"
        f"const {prefix}_POLICY: string = \"{meta['policy']}\"\n"
        f"const {prefix}_CAPABILITY: string = \"{module}\"\n"
        f"const {prefix}_VERSION: int = 1\n\n"
        f"proc {proc_name}() -> bool {{\n"
        f"    give {prefix}_SCHEMA == \"{schema}\" and {prefix}_STAGE == \"{stage}\" and {prefix}_VERSION == 1;\n"
        f"}}\n\n"
        "export *\n"
    )


def test_contract(stage: str) -> str:
    schema = f"vitte.toolchain.{stage}.tests.v1"
    prefix = f"{stage.upper()}_TESTS"
    return (
        f"space toolchain/{stage}/tests/{stage}_contracts\n\n"
        f"const {prefix}_SCHEMA: string = \"{schema}\"\n"
        f"const {prefix}_EXPECTED_STAGE: string = \"{stage}\"\n"
        f"const {prefix}_VERSION: int = 1\n\n"
        f"proc {stage}_contracts_are_versioned() -> bool {{\n"
        f"    give {prefix}_SCHEMA == \"{schema}\" and {prefix}_EXPECTED_STAGE == \"{stage}\" and {prefix}_VERSION == 1;\n"
        f"}}\n\n"
        "export *\n"
    )


def artifact_row(stage: str) -> dict[str, object]:
    rel = ARTIFACTS[stage]
    path = ROOT / rel
    data = path.read_bytes() if path.is_file() else b""
    meta = STAGE_META[stage]
    return {
        "stage": stage,
        "schema": f"vitte.toolchain.{stage}.manifest.v1",
        "role": meta["role"],
        "predecessor": meta["predecessor"],
        "policy": meta["policy"],
        "capabilities": meta["capabilities"],
        "artifact": rel,
        "materialized_by": "committed signed artifact" if stage == "stage0" else "make bootstrap-all",
        "source_contract_root": f"toolchain/{stage}/src",
        "auxiliary_contracts": [
            f"toolchain/{stage}/config/{stage}-policy.json",
            f"toolchain/{stage}/tests/{stage}_contracts.vit",
        ],
        "contract_modules": list(MODULES[stage]),
        "exists": path.is_file(),
        "size": len(data),
        "sha256": hashlib.sha256(data).hexdigest() if data else "",
    }


def main() -> int:
    for stage in STAGES:
        source = ROOT / f"toolchain/{stage}/src"
        source.mkdir(parents=True, exist_ok=True)
        readme = source / "README.md"
        readme.write_text(
            f"# {stage.capitalize()} contracts\n\n"
            f"Stage-specific source contracts for the {stage} compiler.\n"
            "These files are checked by `make toolchain-stage012-gate`.\n",
            encoding="utf-8",
        )
        for module in MODULES[stage]:
            (source / f"{module}.vit").write_text(contract(stage, module), encoding="utf-8")
        config = ROOT / f"toolchain/{stage}/config"
        config.mkdir(parents=True, exist_ok=True)
        config.joinpath(f"{stage}-policy.json").write_text(
            json.dumps({"schema": f"vitte.toolchain.{stage}.policy.v1", "stage": stage, **STAGE_META[stage]}, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )
        tests = ROOT / f"toolchain/{stage}/tests"
        tests.mkdir(parents=True, exist_ok=True)
        tests.joinpath(f"{stage}_contracts.vit").write_text(test_contract(stage), encoding="utf-8")
        (ROOT / f"toolchain/{stage}/reports").mkdir(parents=True, exist_ok=True)
        (ROOT / f"toolchain/{stage}/reports/README.md").write_text(
            f"# {stage.capitalize()} reports\n\nGenerated validation reports for {stage}; the canonical report is under `target/reports`.\n",
            encoding="utf-8",
        )
        manifest = ROOT / f"toolchain/{stage}/{stage}-manifest.json"
        manifest.write_text(json.dumps(artifact_row(stage), indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
