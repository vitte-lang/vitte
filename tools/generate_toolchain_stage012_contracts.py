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
ARTIFACTS = {
    "stage0": "toolchain/bootstrap/stage0/macos-arm64/vitte",
    "stage1": "target/stage1/vitte",
    "stage2": "target/stage2/vitte",
}


def contract(stage: str, module: str) -> str:
    schema = f"vitte.toolchain.{stage}.{module}.v1"
    proc_name = f"{stage}_{module}_contract_ready".replace("-", "_")
    return (
        f"space toolchain/{stage}/{module}\n\n"
        f"const {stage.upper()}_{module.upper()}_SCHEMA: string = \"{schema}\"\n\n"
        f"proc {proc_name}() -> bool {{\n"
        f"    give {stage.upper()}_{module.upper()}_SCHEMA == \"{schema}\";\n"
        f"}}\n\n"
        "export *\n"
    )


def artifact_row(stage: str) -> dict[str, object]:
    rel = ARTIFACTS[stage]
    path = ROOT / rel
    data = path.read_bytes() if path.is_file() else b""
    return {
        "stage": stage,
        "schema": f"vitte.toolchain.{stage}.manifest.v1",
        "artifact": rel,
        "materialized_by": "committed signed artifact" if stage == "stage0" else "make bootstrap-all",
        "source_contract_root": f"toolchain/{stage}/src",
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
        manifest = ROOT / f"toolchain/{stage}/{stage}-manifest.json"
        manifest.write_text(json.dumps(artifact_row(stage), indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
